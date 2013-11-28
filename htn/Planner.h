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
template <typename StateType, typename PlanType>
class PlannerTrace
{
public:
    virtual ~PlannerTrace() {}
    virtual void begin() = 0;
    virtual void end(const optional<PlanType>& result) = 0;
    virtual void pushContext(std::string context, const StateType& state, std::string file, int line) = 0;
    virtual void popContext() = 0;
    virtual void primitive(std::string name, const StateType& state, std::string file, int line) = 0;
    virtual void fail(std::string file, int line) = 0;

    class AutoContext
    {
    public:
        AutoContext(PlannerTrace& trace, const std::string& context, const StateType& state, std::string file, int line)
            : m_trace(&trace)
        {
            m_trace->pushContext(context, state, std::move(file), line);
        }

        ~AutoContext()
        {
            m_trace->popContext();
        }

        PlannerTrace* m_trace;
    };
};

template <typename StateType, typename PlanType>
class NullPlannerTrace : public PlannerTrace<StateType, PlanType>
{
public:
    virtual void begin() override {}
    virtual void end(const optional<PlanType>&) override {}
    virtual void pushContext(std::string, const StateType&, std::string, int) override {}
    virtual void popContext() override {}
    virtual void primitive(std::string, const StateType&, std::string, int) override {}
    virtual void fail(std::string, int) override {}
};

template <typename StateType, typename PlanType>
class StdoutPlannerTrace : public PlannerTrace<StateType, PlanType>
{
public:
    virtual void begin() override
    {
    }

    virtual void end(const optional<PlanType>& result) override
    {
        if (!result)
        {
            std::cout << "Planning failed!" << std::endl;
        }
        else
        {
            std::cout << "Planning succeeded! ";
            // const auto& tasks = result.get().first;
            // std::copy(std::begin(tasks), std::end(tasks), std::ostream_iterator<std::string>(std::cout, " "));
            std::cout << std::endl;
        }
    }

    virtual void pushContext(std::string context, const StateType& state, std::string file, int line) override
    {
        m_contexts.emplace_back(make_pair(std::move(context), state));
        std::cout << file << "(" << line << ") Planning context: ";
        for (auto c : m_contexts)
        {
            std::cout << c.first << " ";
        }
        std::cout << std::endl;
    }

    virtual void popContext() override
    {
        m_contexts.pop_back();
    }

    virtual void primitive(std::string name, const StateType& state, std::string file, int line) override
    {
        m_contexts.emplace_back(make_pair(std::move(name), state));
        std::cout << file << "(" << line << ") Planning context: ";
        for (auto c : m_contexts)
        {
            std::cout << c.first << " ";
        }
        std::cout << std::endl;
    }

    virtual void fail(std::string file, int line) override
    {
        std::cout << file << "(" << line << ") Planning failed: ";
        for (auto context : m_contexts)
        {
            std::cout << context.first << " ";
        }
        std::cout << std::endl;
        std::cout << "(" << m_contexts.rbegin()->second.toString() << ")" << std::endl;
    }

private:
    std::vector<std::pair<std::string, StateType>> m_contexts;
};



#define ENABLE_PLANNER_TRACE
#if defined(ENABLE_PLANNER_TRACE)
#define PLANNER_TRACE_BEGIN(TRACE, DOMAIN) (TRACE).begin()
#define PLANNER_TRACE_END(TRACE, DOMAIN, PLAN) (TRACE).end(PLAN)
#define PLANNER_TRACE_CONTEXT(TRACE, CONTEXT_NAME, STATE) Trace::AutoContext PlannerTraceAutoContext((TRACE), (CONTEXT_NAME), (STATE), __FILE__, __LINE__)
#define PLANNER_TRACE_PRIMITIVE(TRACE, PRIMITIVE_NAME, STATE) (TRACE).primitive((PRIMITIVE_NAME), (STATE), __FILE__, __LINE__)
#define PLANNER_TRACE_FAIL(TRACE) (TRACE).fail(__FILE__, __LINE__)
#else
#define PLANNER_TRACE_BEGIN(TRACE)
#define PLANNER_TRACE_END(TRACE, PLAN)
#define PLANNER_TRACE_CONTEXT(TRACE, CONTEXT_NAME, STATE)
#define PLANNER_TRACE_PRIMITIVE(TRACE, PRIMITIVE_NAME, STATE)
#define PLANNER_TRACE_FAIL(TRACE)
#endif


template <typename StateType, typename PrimitiveType>
class Domain
{
public:
    typedef StateType State;
    typedef PrimitiveType Primitive;

    // A plan is represented as a list of primitives (actions) and the
    // final state after executing the plan.
    typedef std::vector<Primitive> Plan;

    typedef PlannerTrace<State, Plan> Trace;


    //-------------------------------------------------------------------------
    // Generic HTN infrastructure
    //-------------------------------------------------------------------------
    typedef optional<Plan>(Domain::*Task)(State&);

    struct null_action
    {
        optional<Plan> operator()(State&, Trace&)
        {
            return make_optional(Plan());
        }
    };

    static optional<Plan> primitive(Primitive func, Trace&)
    {
        return make_optional(Plan({func}));
    }

    // A methods will search each of the child methods until it finds
    // a branch for which all of the preconditions are met.
    // If none of the methods can succeed then the methods itself fails.
    template <typename Action>
    static optional<Plan> methods(State& state, const State& old_state, Trace& trace)
    {
        auto plan = Action()(state, trace); // test the action
        if (plan)
        {
            return plan; // if a plan can be made for the action return it
        }
        else
        {
            state = old_state;
            PLANNER_TRACE_FAIL(trace);
            return optional<Plan>();
        }
    }

    // A methods will try each of the child methods until it finds a branch
    // which can satisfy the goal. In HTN parlance this are called methods.
    template <typename Action1, typename Action2, typename... Actions>
    static optional<Plan> methods(State& state, const State& old_state, Trace& trace)
    {
        auto plan = Action1()(state, trace); // test the first action
        if (plan)
        {
            return plan; // if a plan can be made for the first task return it
        }
        else
        {
            state = old_state; // reset the state before trying the next method
            return methods<Action2, Actions...>(state, old_state, trace); // otherwise, recurse
        }
    }

    // Search a task list.
    // The final plan is the concatenation of the plans for each task in the task list.
    // If any of the tasks fail then this method fails.
    template <typename Action>
    static optional<Plan> tasks(State& state, const State& old_state, Trace& trace)
    {
        auto plan = Action()(state, trace); // test the first task
        if (plan)
        {
            return plan;
        }
        else
        {
            // return failure if any of the tasks fail.
            PLANNER_TRACE_FAIL(trace);
            state = old_state;
            return optional<Plan>();
        }
    }

    template <typename Action1, typename Action2, typename... Actions>
    static optional<Plan> tasks(State state, const State& old_state, Trace& trace)
    {
        auto plan = Action1()(state, trace); // test the first task
        if (plan)
        {
            // recursively expand the remaining task list
            auto tail_plan = tasks<Action2, Actions...>(state, old_state, trace);
            if (tail_plan)
            {
                // join the plans of each of the tasks.
                std::copy(std::cbegin(tail_plan.get()), std::cend(tail_plan.get()), std::back_inserter(plan.get()));
                return plan;
            }
        }
        // return failure if any of the tasks fail.
        PLANNER_TRACE_FAIL(trace);
        state = old_state;
        return optional<Plan>();
    }



    //--------------------------------------------------------------------------
    #define _begin(NAME)                                                       \
        struct NAME                                                            \
        {                                                                      \
            optional<Plan> operator()(State& state, Trace& trace)              \
            {                                                                  \
                PLANNER_TRACE_CONTEXT(trace, #NAME, state);

    #define _variable(NAME, EXPR)                                              \
                auto NAME = (EXPR);

    #define _precondition(EXPR)                                                \
                if (!(EXPR))                                                   \
                {                                                              \
                    PLANNER_TRACE_FAIL(trace);                                 \
                    return optional<Plan>();                                   \
                }   

    // don't mix operations and methods or tasks
    // we don't correctly roll back the state change if this is done before
    // a methods or tasks call.
    #define _operation(EXPR)                                                   \
                {                                                              \
                    EXPR;                                                      \
                }   

    #define _methods(...)                                                      \
                State old_state = state;                                       \
                return methods<__VA_ARGS__>(state, old_state, trace);

    #define _tasks(...)                                                        \
                State old_state = state;                                       \
                return tasks<__VA_ARGS__>(state, old_state, trace);

    #define _primitive(PRIMITIVE)                                              \
                PLANNER_TRACE_PRIMITIVE(trace, #PRIMITIVE, state);             \
                return primitive(PRIMITIVE, trace);

    #define _end                                                               \
            }                                                                  \
        };

}; // class Planner


// Search the domain for a valid plan.
template <typename DomainType, typename RootAction, typename TraceType>
optional<typename DomainType::Plan> findPlan(typename DomainType::State state, TraceType& trace)
{
    PLANNER_TRACE_BEGIN(trace, domain);
    auto plan = RootAction()(state, trace);
    PLANNER_TRACE_END(trace, domain, plan);
    return plan;
}

#define _findPlan(DOMAIN, ACTION, STATE, TRACE) findPlan<DOMAIN, DOMAIN::ACTION>((STATE), (TRACE));


#endif // PLANNER_H
