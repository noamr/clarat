/*Copyright (c) 2016, Noam Rosenthal
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "qstate-clarat.h"
#include <QStateMachine>
#include <QSignalTransition>
#include <QFinalState>

using namespace qstate_clarat;

namespace {
int sm_event_type()
{
    static int t = QEvent::registerEventType();
    return t;
}

struct StateMachineEvent : public QEvent
{
    StateMachineEvent(std::type_index i)
        : QEvent((QEvent::Type)sm_event_type())
        , id(i)
    {
    }

    std::type_index id;
};
}
namespace qstate_clarat {
class ConditionalTransition : public QAbstractTransition {
public:
    static const std::type_index NullEvent;
    bool eventTest(QEvent *event) override {
        if (event && eventID != NullEvent && event->type() != sm_event_type())
            return false;
        if (eventID != NullEvent && eventID != ((StateMachineEvent*)event)->id)
            return false;
        if (condition && !condition())
            return false;

        return true;
    }

    void onTransition(QEvent*) override {
    }

    ConditionalTransition(std::type_index id, const std::function<bool()>& c)
        : condition(c), eventID(id)
    {

    }

    std::function<bool()> condition;
    std::type_index eventID = NullEvent;
};
const auto ConditionalTransition::NullEvent = std::type_index(typeid(decltype(nullptr)));

StateBase::StateBase()
    : state(new QState)
{
}

void StateBase::declare(Handle h)
{
    *h = state;
}

void StateBase::declare(decltype(Initial))
{
    this->initial = true;
}

void StateBase::declare(Final f)
{
    *f.h = new QFinalState(state);
}

void StateBase::declare(OnEntry e)
{
    QObject::connect(state, &QState::entered, new Invoker(e.f, state), &Invoker::run);
}

void StateBase::declare(OnExit e)
{
    QObject::connect(state, &QState::exited, new Invoker(e.f, state), &Invoker::run);
}

void StateBase::declare(decltype(Parallel))
{
    state->setChildMode(QState::ParallelStates);
}

void StateBase::declare(Property p)
{
    state->assignProperty(p.object, p.property, p.value);
}

void StateBase::declare(StateBase s)
{
    s.state->setParent(state);
    if (s.initial)
        state->setInitialState(s.state);
}

void StateBase::declare(TransitionBase t)
{
    QAbstractTransition* tr;
    if (t.signal.signal && t.signal.source)
        tr = new QSignalTransition(t.signal.source, t.signal.signal);
    else if (t.onDone)
        tr = new QSignalTransition(state, SIGNAL(finished()));
    else
        tr = new ConditionalTransition(t.eventID, t.condition);
    for (const auto& trigger : t.triggers)
        QObject::connect(tr, &QAbstractTransition::triggered, new Invoker(trigger, tr), &Invoker::run);

    QList<QVariant> l;
    for (const auto& ta : t.targets)
        l.push_back(QVariant::fromValue<void*>(ta));
    tr->setProperty("targets", QVariant(l));
    state->addTransition(tr);
}

TransitionBase::TransitionBase()
{
}

void TransitionBase::declare(EventID i)
{
    eventID = i.i;
}

void TransitionBase::declare(Condition c)
{
    condition = c.t;
}

void TransitionBase::declare(Target target)
{
    targets.push_back(target.h);
}

void TransitionBase::declare(Trigger trigger)
{
    triggers.push_back(trigger.f);
}

static void resolveTargets(const QState* s)
{
    for (QAbstractTransition* t : s->transitions()) {
        QList<QAbstractState*> targets;
        auto ta = t->property("targets").value<QList<QVariant>>();
        for (QVariant v: ta) {
            auto h = (Handle)v.value<void*>();
            if (h && *h)
                targets.push_back(*h);
        }
        t->setTargetStates(targets);
    }

    for (const QObject* o : s->children()) {
        auto st = qobject_cast<const QState*>(o);
        if (st)
            resolveTargets(st);
    }
}

void TransitionBase::declare(decltype(OnDone))
{
    onDone = true;
}


void raise(QStateMachine* sm, std::type_index i)
{
    sm->postEvent(new StateMachineEvent(i));
}

}
QStateMachine* createStateMachine(StateBase&& s)
{
    // resolve state IDs;
    auto st = s.qstate();
    resolveTargets(st);
    auto qs = new QStateMachine();
    qs->addState(st);
    qs->setInitialState(st);
    return qs;
}



