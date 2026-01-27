#include <SanctiaEntity.hpp>

void Target::setTarget(int i){target = Component<state3D>::elements[i].entity;};

float & AgentProfile::operator[](const std::string_view & category, const std::string_view & value)
{
    return AgentProfileBase::operator[](category.data())[value.data()];
}

AgentProfile& AgentProfile::operator+(const AgentProfile& other)
{
    for(auto &i : other)
    for(auto &j : i.second)
    {
        operator[](i.first, j.first) = j.second;
    }

    return *this;
}

std::string AgentProfile::toString()
{
    std::string res = "Agent Profile :";

    for(auto &i : *this)
    {
        res += "\n\t";
        res += i.first;
        for(auto &j : i.second)
        {
            res += "\n\t\t";
            res += j.first;
            res += " : ";
            res += std::to_string(j.second);
        }
    }
    res += "\n";
    return res;
}