#pragma once

// No more std::3shn Ali
//#define MRK_STD ::std::

#define MRK_NS mrklang
#define MRK_NS_BEGIN namespace MRK_NS {
#define MRK_NS_BEGIN_MODULE(mod) namespace MRK_NS :: ##mod {
#define MRK_NS_END }