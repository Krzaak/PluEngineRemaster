vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO thedmd/imgui-node-editor
    REF v${VERSION}
    SHA512 83573b6ed776095837373bc95be1c1f5b85e9c5fae2145647f9cb6fdc17d3889edce716ac9e27c1bbde56f00803a66db98ca856910e6e0ce8714d3c5ce3f7c3f
    HEAD_REF master
    PATCHES
        fix-vec2-math-operators.patch
        remove-getkeyindex.patch # GetKeyIndex() is a no-op since 1.87; see https://github.com/ocornut/imgui/issues/5979#issuecomment-1345349492
        fix-imgui-v1.92.5.patch
)

# ImGui 1.92 usunęło ImRect::Floor(); ten port (v0.9.3) go jeszcze woła. Podmieniamy
# wywołania na lokalny helper PluFloorRect() (ImFloor per-składowa). Helper wstawiamy po
# bloku includów. Kolejność zamian: najpierw warianty z prefiksem (node->/m_CurrentPin->),
# żeby uniknąć kolizji podłańcuchów przy gołych m_GroupBounds/m_Bounds.
set(_ne_cpp "${SOURCE_PATH}/imgui_node_editor.cpp")
vcpkg_replace_string("${_ne_cpp}"
    "# include <type_traits>"
    "# include <type_traits>\n\nnamespace { static inline void PluFloorRect(ImRect& r) {\n    r.Min.x = ImFloor(r.Min.x); r.Min.y = ImFloor(r.Min.y);\n    r.Max.x = ImFloor(r.Max.x); r.Max.y = ImFloor(r.Max.y); } }")
vcpkg_replace_string("${_ne_cpp}" "node->m_GroupBounds.Floor();"     "PluFloorRect(node->m_GroupBounds);")
vcpkg_replace_string("${_ne_cpp}" "node->m_Bounds.Floor();"          "PluFloorRect(node->m_Bounds);")
vcpkg_replace_string("${_ne_cpp}" "m_CurrentPin->m_Bounds.Floor();"  "PluFloorRect(m_CurrentPin->m_Bounds);")
vcpkg_replace_string("${_ne_cpp}" "m_GroupBounds.Floor();"           "PluFloorRect(m_GroupBounds);")
vcpkg_replace_string("${_ne_cpp}" "m_NodeRect.Floor();"              "PluFloorRect(m_NodeRect);")
vcpkg_replace_string("${_ne_cpp}" "newBounds.Floor();"               "PluFloorRect(newBounds);")

file(COPY "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" DESTINATION "${SOURCE_PATH}")
file(REMOVE_RECURSE "${SOURCE_PATH}/external/imgui")

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS_DEBUG
        -DIMGUI_NODE_EDITOR_SKIP_HEADERS=ON
)

vcpkg_cmake_install()

vcpkg_copy_pdbs()
vcpkg_cmake_config_fixup(PACKAGE_NAME unofficial-${PORT} CONFIG_PATH share/unofficial-${PORT})

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
