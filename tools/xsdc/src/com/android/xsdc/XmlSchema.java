/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.xsdc;

import com.android.xsdc.tag.*;

import java.util.Collections;
import java.util.List;
import java.util.Map;

public class XmlSchema {
    final private Map<String, XsdElement> elementMap;
    final private Map<String, XsdType> typeMap;
    final private Map<String, XsdAttribute> attributeMap;
    final private Map<String, XsdAttributeGroup> attributeGroupMap;
    final private Map<String, XsdGroup> groupMap;
    final private List<String> includeList;

    XmlSchema(Map<String, XsdElement> elementMap, Map<String, XsdType> typeMap,
            Map<String, XsdAttribute> attributeMap,
            Map<String, XsdAttributeGroup> attributeGroupMap,
            Map<String, XsdGroup> groupMap, List<String> includeList) {
        this.elementMap = elementMap;
        this.typeMap = typeMap;
        this.attributeMap = attributeMap;
        this.attributeGroupMap = attributeGroupMap;
        this.groupMap = groupMap;
        this.includeList = includeList;
    }

    public Map<String, XsdElement> getElementMap() {
        return elementMap;
    }

    public Map<String, XsdType> getTypeMap() {
        return typeMap;
    }

    public Map<String, XsdAttribute> getAttributeMap() {
        return attributeMap;
    }

    public Map<String, XsdAttributeGroup> getAttributeGroupMap() {
        return attributeGroupMap;
    }

    public Map<String, XsdGroup> getGroupMap() {
        return groupMap;
    }

    public List<String> getIncludeList() {
        return includeList;
    }

    public void include(XmlSchema schema) {
        elementMap.putAll(schema.getElementMap());
        typeMap.putAll(schema.getTypeMap());
        attributeMap.putAll(schema.getAttributeMap());
        attributeGroupMap.putAll(schema.getAttributeGroupMap());
        groupMap.putAll(schema.getGroupMap());
    }
}
