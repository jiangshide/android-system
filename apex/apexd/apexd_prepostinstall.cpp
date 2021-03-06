/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "apexd"

#include "apexd_prepostinstall.h"

#include <algorithm>
#include <vector>

#include <fcntl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <android-base/logging.h>
#include <android-base/macros.h>
#include <android-base/scopeguard.h>
#include <android-base/strings.h>

#include "apex_database.h"
#include "apex_file.h"
#include "apex_manifest.h"
#include "apexd.h"
#include "apexd_private.h"
#include "apexd_utils.h"
#include "string_log.h"

using android::base::Error;
using android::base::Result;
using ::apex::proto::ApexManifest;

namespace android {
namespace apex {

namespace {

using MountedApexData = MountedApexDatabase::MountedApexData;

void CloseSTDDescriptors() {
  // exec()d process will reopen STD* file descriptors as
  // /dev/null
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
}

template <typename Fn>
Result<void> StageFnInstall(const std::vector<ApexFile>& apexes,
                            const std::vector<std::string>& mount_points, Fn fn,
                            const char* arg, const char* name) {
  // TODO(b/158470023): consider supporting a session with more than one
  //   pre-install hook.
  int hook_idx = -1;
  for (size_t i = 0; i < apexes.size(); i++) {
    if (!(apexes[i].GetManifest().*fn)().empty()) {
      if (hook_idx != -1) {
        return Error() << "Missing support for multiple " << name << " hooks";
      }
      hook_idx = i;
    }
  }
  CHECK(hook_idx != -1);
  LOG(VERBOSE) << name << " for " << apexes[hook_idx].GetPath();

  // Create invocation args.
  std::vector<std::string> args{
      "/system/bin/apexd", arg,
      mount_points[hook_idx]  // Make the APEX with hook first.
  };
  for (size_t i = 0; i < mount_points.size(); i++) {
    if ((int)i != hook_idx) {
      args.push_back(mount_points[i]);
    }
  }

  return ForkAndRun(args);
}

template <typename Fn>
int RunFnInstall(char** in_argv, Fn fn, const char* name) {
  std::vector<std::string> activation_dirs;
  auto preinstall_guard = android::base::make_scope_guard([&]() {
    for (const std::string& active_point : activation_dirs) {
      if (0 != rmdir(active_point.c_str())) {
        PLOG(ERROR) << "Could not delete temporary active point "
                    << active_point;
      }
    }
  });

  // 1) Unshare.
  if (unshare(CLONE_NEWNS) != 0) {
    PLOG(ERROR) << "Failed to unshare() for apex " << name;
    _exit(200);
  }

  // 2) Make everything private, so that our (and hook's) changes do not
  //    propagate.
  if (mount(nullptr, "/", nullptr, MS_PRIVATE | MS_REC, nullptr) == -1) {
    PLOG(ERROR) << "Failed to mount private.";
    _exit(201);
  }

  std::string hook_path;
  {
    auto bind_fn = [&fn, name,
                    activation_dirs](const std::string& mount_point) mutable {
      std::string hook;
      std::string active_point;
      {
        Result<ApexManifest> manifest_or =
            ReadManifest(mount_point + "/" + kManifestFilenamePb);
        if (!manifest_or.ok()) {
          LOG(ERROR) << "Could not read manifest from  " << mount_point << "/"
                     << kManifestFilenamePb << " for " << name << ": "
                     << manifest_or.error();
          // Fallback to Json manifest if present.
          LOG(ERROR) << "Trying to find a JSON manifest";
          manifest_or = ReadManifest(mount_point + "/" + kManifestFilenameJson);
          if (!manifest_or.ok()) {
            LOG(ERROR) << "Could not read manifest from  " << mount_point << "/"
                       << kManifestFilenameJson << " for " << name << ": "
                       << manifest_or.error();
            _exit(202);
          }
        }
        const auto& manifest = *manifest_or;
        hook = (manifest.*fn)();
        active_point = apexd_private::GetActiveMountPoint(manifest);
        // Ensure there is an activation point. If not, create one and delete
        // later.
        if (0 == mkdir(active_point.c_str(), kMkdirMode)) {
          activation_dirs.push_back(active_point);
        } else if (errno != EEXIST) {
          PLOG(ERROR) << "Unable to create mount point " << active_point;
          _exit(205);
        }
      }

      // 3) Activate the new apex.
      Result<void> bind_status =
          apexd_private::BindMount(active_point, mount_point);
      if (!bind_status.ok()) {
        LOG(ERROR) << "Failed to bind-mount " << mount_point << " to "
                   << active_point << ": " << bind_status.error();
        _exit(203);
      }

      return std::make_pair(active_point, hook);
    };

    // First/main APEX.
    auto [active_point, hook] = bind_fn(in_argv[2]);
    hook_path = active_point + "/" + hook;

    for (size_t i = 3;; ++i) {
      if (in_argv[i] == nullptr) {
        break;
      }
      bind_fn(in_argv[i]);  // Ignore result, hook will be empty.
    }
  }

  // 4) Run the hook.

  // For now, just run sh. But this probably needs to run the new linker.
  std::vector<std::string> args{
      hook_path,
  };
  std::vector<const char*> argv;
  argv.resize(args.size() + 1, nullptr);
  std::transform(args.begin(), args.end(), argv.begin(),
                 [](const std::string& in) { return in.c_str(); });

  LOG(ERROR) << "execv of " << android::base::Join(args, " ");

  // Close all file descriptors. They are coming from the caller, we do not
  // want to pass them on across our fork/exec into a different domain.
  CloseSTDDescriptors();

  execv(argv[0], const_cast<char**>(argv.data()));
  PLOG(ERROR) << "execv of " << android::base::Join(args, " ") << " failed";
  _exit(204);
}

}  // namespace

Result<void> StagePreInstall(const std::vector<ApexFile>& apexes,
                             const std::vector<std::string>& mount_points) {
  return StageFnInstall(apexes, mount_points, &ApexManifest::preinstallhook,
                        "--pre-install", "pre-install");
}

int RunPreInstall(char** in_argv) {
  return RunFnInstall(in_argv, &ApexManifest::preinstallhook, "pre-install");
}

Result<void> StagePostInstall(const std::vector<ApexFile>& apexes,
                              const std::vector<std::string>& mount_points) {
  return StageFnInstall(apexes, mount_points, &ApexManifest::postinstallhook,
                        "--post-install", "post-install");
}

int RunPostInstall(char** in_argv) {
  return RunFnInstall(in_argv, &ApexManifest::postinstallhook, "post-install");
}

}  // namespace apex
}  // namespace android
