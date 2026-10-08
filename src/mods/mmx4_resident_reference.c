#include "mod_plugins.h"
// The fixed REFERENCE build keeps original drive loading. A matching catalog
// activation is registered so plans resolve without installing any HLE hook.
static void activate(void) {}
PSX_MOD_CONSTRUCTOR(mmx4_register_resident_reference) {
    psx_mod_register_activation_plugin("mmx4.resident-loading",activate);
}
