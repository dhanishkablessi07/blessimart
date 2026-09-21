#pragma once
#include <string>

// Mirrors the products table. Plain struct, no ORM.
struct Product
{
    int id = 0;
    int seller_id = 0;
    std::string title;
    std::string description;
    double price = 0.0;
    int stock = 0;
    std::string category = "General";
    std::string image_url;
};
