#include "Planner.h"

#include <string>
#include <iostream>
#include <sstream>

//-----------------------------------------------------------------------------
// This is the actor who will run the plan.
// Primitives in the domain correspond to member functions on this actor.
//-----------------------------------------------------------------------------
class Actor
{
public:
    bool is_hungry {false};
    int  cash {0};
    bool can_cook {false};

    void order_takeout()
    {
        std::cout << "order_takeout" << std::endl;
        cash -= 20;
    }

    void cook_dinner()
    {
        std::cout << "cook_dinner" << std::endl;
    }

    void eat_dinner()
    {
        std::cout << "eat_dinner" << std::endl;
        is_hungry = false;
    }

    void wash_dishes()
    {
        std::cout << "wash_dishes" << std::endl;
    }

    void watch_tv()
    {
        std::cout << "watch_tv" << std::endl;
    }

    std::string toString() const
    {
        std::ostringstream os;
        os << "is_hungry: " << (is_hungry ? "true" : "false") << ", ";
        os << "can_cook: " << (can_cook ? "true" : "false") << ", ";
        os << "cash: " << cash;
        return os.str();
    }
};

//-----------------------------------------------------------------------------
// State information for the planner
//-----------------------------------------------------------------------------
class DinnerState
{
public:
    bool actor_is_hungry {false};
    bool actor_can_cook {false};
    int  actor_cash {0};
    bool food_in_fridge {false};
    bool dishes {false};

    std::string toString() const
    {
        std::ostringstream os;
        os << "actor_is_hungry: " << (actor_is_hungry ? "true" : "false") << ", ";
        os << "actor_can_cook: " << (actor_can_cook ? "true" : "false") << ", ";
        os << "actor_cash: " << actor_cash << ", ";
        os << "food_in_fridge: " << (food_in_fridge ? "true" : "false") << ", ";
        os << "dishes: " << dishes;
        return os.str();
    }
};

//-----------------------------------------------------------------------------
// The definition of the planner Domain
//-----------------------------------------------------------------------------
class DinnerDomain : public Domain<DinnerState, void(Actor::*)()>
{
public:
    _begin(order_takeout)
        _variable(cost, 20)
        _precondition(state.actor_cash >= cost)
        _operation(state.actor_cash -= cost)
        _primitive(&Actor::order_takeout)
    _end

    _begin(cook_dinner)
        _precondition(state.actor_can_cook)
        _precondition(state.food_in_fridge)
        _operation(state.food_in_fridge = false; state.dishes = true)
        _primitive(&Actor::cook_dinner)
    _end

    _begin(eat_dinner)
        _operation(state.actor_is_hungry = false)
        _primitive(&Actor::eat_dinner)
    _end

    _begin(wash_dishes)
        _precondition(state.dishes)
        _operation(state.dishes = false)
        _primitive(&Actor::wash_dishes)
    _end

    _begin(get_dinner)
        _methods(cook_dinner, order_takeout)
    _end

    _begin(clean_up)
        _methods(wash_dishes, null_action)
    _end

    _begin(have_dinner)
        _precondition(state.actor_is_hungry)
        _tasks(get_dinner, eat_dinner, clean_up)
    _end

    _begin(watch_tv)
        _primitive(&Actor::watch_tv)
    _end

    _begin(do_something)
        _methods(have_dinner, watch_tv)
    _end
};


//-----------------------------------------------------------------------------
// Execute each of the actions in the plan.
//-----------------------------------------------------------------------------
void executePlan(Actor& actor, const DinnerDomain::Plan& plan)
{
    for (auto primitive : plan)
    {
        (actor.*primitive)();
    }
}


//-----------------------------------------------------------------------------
int main()
{
    Actor actor;
    actor.is_hungry = true;
    actor.cash = 30;
    actor.can_cook = false;

    std::cout << "Actor state: " << actor.toString() << std::endl;

    DinnerState state;
    state.actor_is_hungry = actor.is_hungry;
    state.actor_can_cook = actor.can_cook;
    state.actor_cash = actor.cash;
    state.food_in_fridge = true;
    state.dishes = false;

    // Use either StdoutPlannerTrace or NullPlannerTrace
    // NullPlannerTrace<DinnerState, typename DinnerDomain::Plan> trace;
    StdoutPlannerTrace<DinnerState, typename DinnerDomain::Plan> trace;

    auto plan = _findPlan(DinnerDomain, do_something, state, trace);
    if (plan)
    {
        executePlan(actor, plan.get());
    }

    std::cout << "Actor state: " << actor.toString() << std::endl;

    return 0;
} 