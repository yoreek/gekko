#include "platform/LittleFsBlobStore.h"

namespace ewfm {

namespace {
LittleFsBlobStore* g_defaultBlobStore = nullptr;
} // namespace

LittleFsBlobStore* defaultBlobStore() {
    return g_defaultBlobStore;
}

void setDefaultBlobStore(LittleFsBlobStore* store) {
    g_defaultBlobStore = store;
}

} // namespace ewfm
