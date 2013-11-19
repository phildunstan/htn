#ifndef PLANNER_H
#define PLANNER_H

#include <string>
#include <algorithm>
#include <vector>
#include <utility>
#include <functional>
#include <iostream>

#include "optional.h"


//-----------------------------------------------------------------------------
// Tracing Framework
//-----------------------------------------------------------------------------
template <typename Domain>
class PlannerTrace
{
public:
    static PlannerTrace& getInstance()
    {
        static PlannerTrace trace;
        return trace;
    }

    void begin()
    {
    }

    void end(const optional<typename Domain::Plan>& result)
    {
        if (!result)
        {
            std::cout << "Planning failed!" << std::endl;
        }
        else
        {
            const auto& tasks = result.get().first;
            std::cout << "Planning succeeded! ";
            std::copy(std::begin(tasks), std::end(tasks), std::ostream_iterator<std::string>(std::cout, " "));
            std::cout << std::endl;
        }
    }

    void pushContext(std::string context, typename Domain::State state, std::string file, int line)
    {
        m_contexts.emplace_back(make_pair(std::move(context), state));
        std::cout << file << "(" << line << ") Planning context: ";
        for (auto c : m_contexts)
        {
            std::cout << c.first << " ";
        }
        std::cout << std::endl;
    }

    void popContext()
    {
        m_contexts.pop_back();
    }

    void fail(std::string file, int line)
    {
        std::cout << file << "(" << line << ") Planning failed: ";
        for (auto context : m_contexts)
        {
            std::cout << context.first << " ";
        }
        std::cout << std::endl;
        std::cout << "(" << m_contexts.rbegin()->second.toString() << ")" << std::endl;
    }

    class AutoContext
    {
    public:
        AutoContext(const std::string& context, typename Domain::State state, std::string file, int line)
        {
            PlannerTrace::getInstance().pushContext(context, std::move(state), std::move(file), line);
        }

        ~AutoContext()
        {
            PlannerTrace::getInstance().popContext();
        }
    };

private:
    PlannerTrace()
    {
    }

    std::vector<std::pair<std::string, typename Domain::State>> m_contexts;
};

#define PLANNER_TRACE_BEGIN() Trace::getInstance().begin()
#define PLANNER_TRACE_END(RESULT) Trace::getInstance().end(RESULT)
#define PLANNER_TRACE_CONTEXT(CONTEXT_NAME, STATE) Trace::AutoContext PlannerTraceAutoContext(CONTEXT_NAME, STATE, __FILE__, __LINE__)
#define PLANNER_TRACE_FAIL() Trace::getInstance().fail(__FILE__, __LINE__)



template <typename StateType>
class Domain
{
public:
    typedef StateType State;

    // A plan is represented as a string of operators (actions) and the
    // final state after executing the plan.
    typedef std::pair<std::vector<std::string>, State> Plan;

    typedef PlannerTrace<Domain<State>> Trace;


    //-------------------------------------------------------------------------
    // Generic HTN infrastructure
    //-------------------------------------------------------------------------

    struct null_action
    {
        optional<Plan> operator()(State state)
        {
            return optional<Plan>(Plan{std::vector<std::string>{}, state});
        }
    };

    static optional<Plan> simple_postcondition(State state, std::string name)
    {
        return make_optional(Plan(std::vector<std::string>{name}, state));
    }

    // A selector will search each of the child branches until it finds
    // a branch for which all of the preconditions are met.
    // If none of the branches can succeed then the selector itself fails.
    template <typename Action>
    static optional<Plan> selector(State state)
    {
        auto plan = Action()(state); // test the action
        if (plan)
        {
            return plan; // if a plan can be made for the action return it
        }
        else
        {
            PLANNER_TRACE_FAIL();
            return optional<Plan>();
        }
    }

    // A selector will try each of the child branches until it finds a branch
    // which can satisfy the goal. In HTN parlance this are called methods.
    template <typename Action1, typename Action2, typename... Actions>
    static optional<Plan> selector(State state)
    {
        auto plan = Action1()(state); // test the first action
        if (plan)
        {
            return plan; // if a plan can be made for the first action return it
        }
        else
        {
            return selector<Action2, Actions...>(state); // otherwise, recurse
        }
    }

    // A sequence will search each of the child branches sequentially.
    // The final plan is the concatenation of the plans for each child branch.
    // If any of the branches fail then the sequence itself fails.
    template <typename Action>
    static optional<Plan> sequence(State state)
    {
        auto plan = Action()(state); // expand the first child
        if (plan)
        {
            return plan;
        }
        else
        {
            // return failure if any of the branches fail.
            PLANNER_TRACE_FAIL();
            return optional<Plan>();
        }
    }

    template <typename Action1, typename Action2, typename... Actions>
    static optional<Plan> sequence(State state)
    {
        auto plan = Action1()(state); // expand the first child
        if (plan)
        {
            // recursively expand the remaining branches
            auto tail_plan = sequence<Action2, Actions...>(plan.get().second);
            if (tail_plan)
            {
                // join the plans of each of the child branches.
                std::copy(std::begin(tail_plan.get().first), std::end(tail_plan.get().first), std::back_inserter(plan.get().first));
                return make_optional(Plan{plan.get().first, tail_plan.get().second});
            }
        }
        // return failure if any of the branches fail.
        PLANNER_TRACE_FAIL();
        return optional<Plan>();
    }



    //--------------------------------------------------------------------------
    #define _begin(NAME)                                                       \
        struct NAME                                                            \
        {                                                                      \
            optional<Plan> operator()(State state)                             \
            {                                                                  \
                PLANNER_TRACE_CONTEXT(#NAME, state);

    #define _variable(NAME, EXPR)                                              \
        auto NAME = (EXPR);

    #define _precondition(EXPR)                                                \
        if (!(EXPR))                                                           \
        {                                                                      \
            PLANNER_TRACE_FAIL();                                              \
            return optional<Plan>();                                           \
        }   

    #define _postcondition(EXPR)                                               \
        EXPR;    

    #define _sequence(...)                                                     \
        return sequence<__VA_ARGS__>(state);

    #define _selector(...)                                                     \
        return selector<__VA_ARGS__>(state);

    #define _primitive(NAME)                                                   \
        return simple_postcondition(state, NAME);

    #define _end                                                               \
            }                                                                  \
        };

}; // class Planner


    
template <typename Domain, typename RootAction>
optional<typename Domain::Plan> findPlan(typename Domain::State state)
{
    Domain::PLANNER_TRACE_BEGIN();
    auto plan = RootAction()(state);
    Domain::PLANNER_TRACE_END(plan);
    return plan;
}

#define _findPlan(DOMAIN, ACTION, STATE) findPlan<DOMAIN, DOMAIN::ACTION>(STATE);


#endif // PLANNER_H
