// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * @file content_browser.cpp
 * @brief Content browser panel implementation.
 * @date 03/08/2026
 * @author Coela Can't
 */

#include "content_browser.hpp"

namespace koilo {
namespace ui {

// ============================================================================
// Construction & Navigation
// ============================================================================

// Build the content browser widget hierarchy.
int ContentBrowser::Build(UIContext& ctx, int parentIdx, const char* id) {
    ctx_ = &ctx;

    // Root container (column: breadcrumb row | split)
    rootIdx_ = ctx.CreatePanel(id);
    ctx.SetParent(rootIdx_, parentIdx);
    Widget* root = ctx.Pool().Get(rootIdx_);
    root->layout.direction = LayoutDirection::Column;
    root->widthMode  = SizeMode::Percent; root->localW = 100.0f;
    root->heightMode = SizeMode::Percent; root->localH = 100.0f;

    // -- Top bar: search + breadcrumb --
    char topId[64];
    std::snprintf(topId, sizeof(topId), "%s_top", id);
    topBarIdx_ = ctx.CreatePanel(topId);
    ctx.SetParent(topBarIdx_, rootIdx_);
    Widget* top = ctx.Pool().Get(topBarIdx_);
    top->layout.direction = LayoutDirection::Row;
    top->layout.crossAlign = Alignment::Center;
    top->widthMode  = SizeMode::Percent; top->localW = 100.0f;
    top->heightMode = SizeMode::Fixed;   top->localH = 28.0f;
    top->padding = {2, 2, 4, 4};

    // Search field
    char sfId[64];
    std::snprintf(sfId, sizeof(sfId), "%s_search", id);
    searchIdx_ = ctx.CreateTextField(sfId, "Search...");
    ctx.SetParent(searchIdx_, topBarIdx_);
    Widget* sf = ctx.Pool().Get(searchIdx_);
    sf->widthMode  = SizeMode::Fixed; sf->localW = 160.0f;
    sf->heightMode = SizeMode::Fixed; sf->localH = 22.0f;
    sf->iconId = IconId::Search;

    // Breadcrumb
    char bcId[64];
    std::snprintf(bcId, sizeof(bcId), "%s_bc", id);
    breadcrumb_.Build(ctx, topBarIdx_, bcId);
    breadcrumb_.SetOnNavigate([this](const std::string& path) {
        NavigateTo(path);
    });

    // -- Split: tree (left) | file list (right) --
    char splitId[64];
    std::snprintf(splitId, sizeof(splitId), "%s_split", id);
    splitIdx_ = ctx.CreatePanel(splitId);
    ctx.SetParent(splitIdx_, rootIdx_);
    Widget* split = ctx.Pool().Get(splitIdx_);
    split->layout.direction = LayoutDirection::Row;
    split->widthMode  = SizeMode::Percent; split->localW = 100.0f;
    split->heightMode = SizeMode::Percent; split->localH = 100.0f;
    split->layout.mainAlign = Alignment::Start;

    // Tree sidebar
    char treeId[64];
    std::snprintf(treeId, sizeof(treeId), "%s_tree", id);
    treeIdx_ = ctx.CreateWidget(WidgetTag::ScrollView, treeId);
    ctx.SetParent(treeIdx_, splitIdx_);
    Widget* tree = ctx.Pool().Get(treeIdx_);
    tree->layout.direction = LayoutDirection::Column;
    tree->widthMode  = SizeMode::Fixed;   tree->localW = 180.0f;
    tree->heightMode = SizeMode::Percent; tree->localH = 100.0f;

    // File list
    char listId[64];
    std::snprintf(listId, sizeof(listId), "%s_list", id);
    listIdx_ = ctx.CreateWidget(WidgetTag::ScrollView, listId);
    ctx.SetParent(listIdx_, splitIdx_);
    Widget* list = ctx.Pool().Get(listIdx_);
    list->layout.direction = LayoutDirection::Column;
    list->widthMode  = SizeMode::Percent; list->localW = 100.0f;
    list->heightMode = SizeMode::Percent; list->localH = 100.0f;
    list->heightMode = SizeMode::Percent; list->localH = 100.0f;

    return rootIdx_;
}

// Navigate to a directory path and refresh contents.
void ContentBrowser::NavigateTo(const std::string& path) {
    if (!ctx_) return;
    currentPath_ = path;
    breadcrumb_.SetPath(path);
    RefreshFileList();
    RefreshTree();
}

// ============================================================================
// File List
// ============================================================================

// Refresh the file list from the current path.
void ContentBrowser::RefreshFileList() {
    if (!ctx_ || listIdx_ < 0) return;
    UIContext& ctx = *ctx_;

    // Clear existing children
    Widget* list = ctx.Pool().Get(listIdx_);
    if (!list) return;
    for (int i = 0; i < list->childCount; ++i) {
        int ci = list->children[i];
        if (ci >= 0) ctx.Pool().Free(ci);
    }
    list->childCount = 0;

    // Fetch entries
    entries_ = ListDirectory(currentPath_);

    // Apply search filter
    std::string filter;
    if (searchIdx_ >= 0) {
        Widget* sf = ctx.Pool().Get(searchIdx_);
        if (sf && sf->textId) {
            const char* txt = ctx.Strings().Lookup(sf->textId);
            if (txt) filter = txt;
        }
    }

    for (size_t i = 0; i < entries_.size(); ++i) {
        const FsEntry& e = entries_[i];

        // Filter
        if (!filter.empty() && filter != "Search...") {
            bool match = false;
            for (size_t j = 0; j + filter.size() <= e.name.size(); ++j) {
                bool ok = true;
                for (size_t k = 0; k < filter.size(); ++k) {
                    char ca = e.name[j + k];
                    char cf = filter[k];
                    if (ca >= 'A' && ca <= 'Z') ca += 32;
                    if (cf >= 'A' && cf <= 'Z') cf += 32;
                    if (ca != cf) { ok = false; break; }
                }
                if (ok) { match = true; break; }
            }
            if (!match) continue;
        }

        char rowId[80];
        std::snprintf(rowId, sizeof(rowId), "cb_row_%zu", i);
        int row = ctx.CreatePanel(rowId);
        ctx.SetParent(row, listIdx_);
        Widget* rw = ctx.Pool().Get(row);
        rw->layout.direction = LayoutDirection::Row;
        rw->layout.crossAlign = Alignment::Center;
        rw->widthMode  = SizeMode::Percent; rw->localW = 100.0f;
        rw->heightMode = SizeMode::Fixed;   rw->localH = 22.0f;
        rw->padding = {1, 1, 4, 4};
        rw->iconId = e.icon;

        // Icon label
        char iconId[80];
        std::snprintf(iconId, sizeof(iconId), "cb_ico_%zu", i);
        int ico = ctx.CreateLabel(iconId, IconName(e.icon));
        ctx.SetParent(ico, row);
        Widget* iw = ctx.Pool().Get(ico);
        iw->widthMode = SizeMode::Fixed; iw->localW = 20.0f;
        iw->iconId = e.icon;

        // Name label
        char nameId[80];
        std::snprintf(nameId, sizeof(nameId), "cb_name_%zu", i);
        int name = ctx.CreateLabel(nameId, e.name.c_str());
        ctx.SetParent(name, row);
        Widget* nw = ctx.Pool().Get(name);
        nw->widthMode = SizeMode::Percent; nw->localW = 100.0f;

        // Size label (files only)
        if (!e.isDir && e.size > 0) {
            char sizeId[80];
            std::snprintf(sizeId, sizeof(sizeId), "cb_sz_%zu", i);
            std::string sizeStr = FormatSize(e.size);
            int sz = ctx.CreateLabel(sizeId, sizeStr.c_str());
            ctx.SetParent(sz, row);
            Widget* sw = ctx.Pool().Get(sz);
            sw->widthMode = SizeMode::Fixed; sw->localW = 60.0f;
        }

        // Click handler: navigate into directories, trigger callback for files
        size_t entryIdx = i;
        rw->flags.focusable = 1;
        rw->onClickCpp = [this, entryIdx](Widget&) {
            if (entryIdx < entries_.size()) {
                const FsEntry& entry = entries_[entryIdx];
                if (entry.isDir) {
                    NavigateTo(entry.path);
                } else if (onFileAction_) {
                    onFileAction_(entry);
                }
            }
        };

        // Drag source for files (asset drag)
        if (!e.isDir) {
            rw->acceptDropTypes = 0; // not a drop target
            size_t dragIdx = i;
            rw->onDragBegin = [this, dragIdx](int /*widgetIdx*/) -> DragPayload {
                if (dragIdx >= entries_.size()) return {};
                const FsEntry& entry = entries_[dragIdx];
                DragPayload payload;
                payload.typeTag = DragType::Asset;
                payload.data = &entries_[dragIdx];
                payload.sourceWidget = -1;
                if (ctx_) payload.labelId = ctx_->Strings().Intern(entry.name.c_str());
                return payload;
            };
        }
    }
}

// ============================================================================
// Directory Tree
// ============================================================================

// Refresh the directory tree sidebar.
void ContentBrowser::RefreshTree() {
    if (!ctx_ || treeIdx_ < 0) return;
    UIContext& ctx = *ctx_;

    // Clear
    Widget* tree = ctx.Pool().Get(treeIdx_);
    if (!tree) return;
    for (int i = 0; i < tree->childCount; ++i) {
        int ci = tree->children[i];
        if (ci >= 0) ctx.Pool().Free(ci);
    }
    tree->childCount = 0;

    // List subdirectories only for tree
    auto allEntries = ListDirectory(currentPath_);
    int dirCount = 0;
    for (const auto& e : allEntries) {
        if (!e.isDir) continue;

        char nodeId[80];
        std::snprintf(nodeId, sizeof(nodeId), "cb_tn_%d", dirCount);
        int tn = ctx.CreateWidget(WidgetTag::TreeNode, nodeId);
        ctx.SetParent(tn, treeIdx_);
        Widget* tw = ctx.Pool().Get(tn);
        tw->textId = ctx.Strings().Intern(e.name.c_str());
        tw->iconId = IconId::Folder;
        tw->widthMode  = SizeMode::Percent; tw->localW = 100.0f;
        tw->heightMode = SizeMode::Fixed;   tw->localH = 22.0f;

        std::string dirPath = e.path;
        tw->onClickCpp = [this, dirPath](Widget&) {
            NavigateTo(dirPath);
        };
        ++dirCount;
    }
}

} // namespace ui
} // namespace koilo
