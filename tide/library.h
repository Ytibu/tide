#ifndef TIDE_LIBRARY_H__
#define TIDE_LIBRARY_H__

#include <memory>
#include "module.h"

namespace tide {

class Library {
public:
    static Module::ptr GetModule(const std::string& path);
};

}

#endif // TIDE_LIBRARY_H__
