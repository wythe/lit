#pragma once

#include "ln_rpc.h"
#include "bc_rpc.h"
#include "web_rpc.h"

namespace lit {
struct hosts {
	lit::ld ld;
	lit::bd bd;
	lit::web::https https; 
};
}
