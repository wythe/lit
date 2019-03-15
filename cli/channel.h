#pragma once
#include <string>
#include <vector>
#include "ln_rpc.h"

namespace lit {
struct hosts;

// fund all channels in connected state.
void fund_all(const hosts &hosts, const channel_list &channels);

void close(const channel_list &channels);

// autopilot algorithm
void autopilot(const hosts &hosts);

/* Bootstrap a new lightning node by connecting to 10 random 1ML nodes
 * and wait for network graph construction.  Then disconnect from them.
 */
void bootstrap(const hosts &hosts);
}
