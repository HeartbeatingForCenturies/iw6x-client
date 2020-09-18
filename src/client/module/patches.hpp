#pragma once
#include "loader/module_loader.hpp"

class patches final : public module
{	
public:
    void post_unpack() override;
    void mp();
    void sp();

private:

};
