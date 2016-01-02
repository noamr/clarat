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
#ifndef QSTATECLARAT_H
#define QSTATECLARAT_H

#include <clarat.h>
#include <QState>
#include <QVariant>
#include <QAbstractTransition>

#include <functional>
#include <typeindex>

namespace qstate_clarat {
struct Property {
    QObject* object;
    const char* property;
    QVariant value;
};

class Invoker : public QObject
{
    Q_OBJECT

public:
    Invoker(const std::function<void()>& func, QObject* o = nullptr) : QObject(o), f(func) {}
    void run() { f(); }

private:
    std::function<void()> f;
};

typedef QAbstractState** Handle;

struct Signal { QObject* source; const char* signal; };
struct Trigger { std::function<void()> f; };
struct OnEntry { std::function<void()> f; };
struct OnExit { std::function<void()> f; };
struct Condition { std::function<bool()> t; };
struct Target { Handle h; };
struct Final { Handle h; };
template<typename T>
struct On { };
struct EventID { std::type_index i; };
enum { Initial };
enum { Parallel };
enum { OnDone };

class TransitionBase;
class StateBase {
public:
    void declare(Property p);
    void declare(TransitionBase t);
    void declare(StateBase);
    void declare(OnExit);
    void declare(OnEntry);
    void declare(Handle h);
    void declare(decltype(Initial));
    void declare(decltype(Parallel));
    void declare(Final);

    QState* qstate() { return state; }

protected:
    StateBase();

private:
    bool initial = false;
    QState* state;
};


class TransitionBase {
public:
    void declare(Target target);
    void delcare(Signal signal);
    void declare(Trigger trigger);
    void declare(Condition cond);
    void declare(EventID);
    void declare(decltype(OnDone));

    template<typename T>
    void declare(On<T>) {
        declare(EventID{ std::type_index(typeid(T)) });
    }

protected:
    TransitionBase();

private:
    friend class StateBase;
    std::vector<Handle> targets;
    Signal signal = { nullptr, nullptr };
    std::vector<std::function<void()>> triggers;
    std::function<bool()> condition;
    bool onDone = false;
    std::type_index eventID = std::type_index(typeid(decltype(nullptr)));
};

typedef clarat::Declarative<StateBase> State;
typedef clarat::Declarative<TransitionBase> Transition;

void raise(QStateMachine* sm, std::type_index i);

}

using qstate_clarat::TransitionBase;
using qstate_clarat::StateBase;

QStateMachine* createStateMachine(StateBase&&);

template<typename T>
void raise(QStateMachine* sm)
{
    qstate_clarat::raise(sm, std::type_index(typeid(T)));
}

#endif // QSTATECLARAT_H

