#include "identity/IdentitySession.hpp"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <cstdlib>
#include <json.hpp>
EM_JS(char*, ReadCombiIdentity, (), {
    return stringToNewUTF8(JSON.stringify(Module['api'].snapshot()));
});
EM_JS_DEPS(combi_identity_deps, "$stringToNewUTF8");
#endif

void IdentitySession::Poll() {
#ifdef __EMSCRIPTEN__
    char* raw = ReadCombiIdentity();
    const auto state = nlohmann::json::parse(raw, nullptr, false);
    std::free(raw);
    m_authenticated = !state.is_discarded() && state.value("status", "local") == "ready";
    m_id = m_authenticated ? state.value("id", "") : "";
    if (!state.is_discarded()) m_epoch = state.value("epoch", std::uint64_t{0});
#endif
}