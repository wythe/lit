#pragma once
#include <string>
#include <vector>
#include "ln_rpc.h"

namespace lit {
struct hosts;

/* Bootstrap a new lightning node by connecting to 10 random 1ML nodes
 * and wait for network graph construction.  Then disconnect from them.
 */
void bootstrap(const hosts &hosts);

// Connect to @count random peers.
void connect(const hosts &hosts, int count);

// Fund @count random channels from current connections.  @count can be
// set to all by using a huge number.
void open_channel(const hosts &hosts, int count, uint64_t sats);

// Close close a list of channels.
void close_channel(const channel_list &channels);

// autopilot algorithm
void autopilot(const hosts &hosts);

}
