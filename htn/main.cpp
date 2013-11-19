#include "Planner.h"

#include <string>
#include <iostream>
#include <sstream>

//-----------------------------------------------------------------------------
// Domain specific state information
//-----------------------------------------------------------------------------
class DinnerState
{
public:
    bool hungry {false};
    bool food_in_fridge {false};
    bool can_cook {false};
    int  cash {0};
    bool dishes {false};

    std::string toString() const
    {
        std::ostringstream os;
        os << "hungry: " << (hungry ? "true" : "false") << ", ";
        os << "food_in_fridge: " << (food_in_fridge ? "true" : "false") << ", ";
        os << "can_cook: " << can_cook << ", ";
        os << "cash: " << cash << ", ";
        os << "dishes: " << dishes;
        return os.str();
    }
};

//-----------------------------------------------------------------------------
class DinnerDomain : public Domain<DinnerState>
{
public:
    _begin(order_takeout)
        _variable(cost, 20);
        _precondition(state.cash >= cost)
        _postcondition(state.cash -= cost)
        _primitive("order_takeout")
    _end

    _begin(cook_dinner)
        _precondition(state.food_in_fridge)
        _precondition(state.can_cook)
        _postcondition(state.food_in_fridge = false)
        _postcondition(state.dishes = true)
        _primitive("cook_dinner")
    _end

    _begin(eat_dinner)
        _postcondition(state.hungry = false)
        _primitive("eat_dinner")
    _end

    _begin(wash_dishes)
        _precondition(state.dishes)
        _postcondition(state.dishes = false)
        _primitive("wash_dishes")
    _end

    _begin(get_dinner)
        _selector(cook_dinner, order_takeout)
    _end

    _begin(clean_up)
        _selector(wash_dishes, null_action)
    _end

    _begin(have_dinner)
        _precondition(state.hungry)
        _sequence(get_dinner, eat_dinner, clean_up)
    _end

    _begin(watch_tv)
        _primitive("watch_tv")
    _end

    _begin(do_something)
        _selector(have_dinner, watch_tv)
    _end
};


//-----------------------------------------------------------------------------
int main()
{
    DinnerState state;
    state.hungry = true;
    state.food_in_fridge = true;
    state.can_cook = true;
    state.cash = 30;

    auto plan = _findPlan(DinnerDomain, do_something, state);

    if (plan)
    {
        for (auto action : plan.get().first)
        {
            std::cout << action << " ";
        }
        std::cout << std::endl;
    }

    return 0;
}