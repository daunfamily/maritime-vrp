//  Created by Alberto Santini on 18/09/13.
//  Copyright (c) 2013 Alberto Santini. All rights reserved.
//

#include <base/node.h>

float Node::pu_demand() const {
    return (pu_type == PickupType::PICKUP ? port->pickup_demand : 0);
}

float Node::de_demand() const {
    return (pu_type == PickupType::DELIVERY ? port->delivery_demand : 0);
}

bool Node::same_row_as(const Node& other) const {
    return (other.port == port && other.pu_type == pu_type);
}

bool Node::operator==(const Node& other) const {
    return (same_row_as(other) && other.time_step == time_step);
}

ostream& operator<<(ostream& out, const PickupType& pu) {
    out << (pu == PickupType::PICKUP ? "pu" : "de");
    return out;
}

ostream& operator<<(ostream& out, const Node& n) {
    out << "[" << n.port->name << ", " << n.pu_type << ", " << n.time_step;
    out << ", dem: " << (n.pu_type == PickupType::PICKUP ? n.pu_demand() : n.de_demand());
    out << "]";
    return out;
}