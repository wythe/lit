#pragma once

#include "ln_rpc.h"
#include "bc_rpc.h"
#include "web_rpc.h"

namespace rpc {
struct hosts {
	rpc::lightning::ld ld;
	rpc::bitcoin::bd bd;
	rpc::web::https https; 
};
}
