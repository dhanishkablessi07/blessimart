#pragma once
#include <string>

// Simple user data shared by controllers.
// Beginner note: this is just a plain struct, no ORM magic.
struct User
{
    int id = 0;
    std::string name;
    std::string email;
    std::string role;  // "buyer", "seller", or "admin"
};
