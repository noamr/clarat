/*
Copyright (c) 2016, Noam Rosenthal
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

#include <QCoreApplication>
#include "qstate-clarat.h"
#include <QStateMachine>
#include <QTimer>

using namespace qstate_clarat;
namespace {
typedef QAbstractState* StateHandle;
StateHandle test1, test1Sub1, test1Sub2, test2;
struct Event1 {};
QStateMachine* doIt()
{
    return createStateMachine(
        State {
            State { Initial, Transition { Target { &test1 } } },
            State { &test1,
                State { Initial, Transition { Target { &test1Sub1 } } },
                OnEntry { [=] { printf("Inside Test1\n"); } },
                State { &test1Sub1,
                    OnEntry { [=] { printf("Inside Test1Sub1\n"); } },
                    OnExit { [=] { printf("Leaving Test1Sub1\n"); } },
                    Transition { On<Event1>(), Target { &test1Sub2 } }
                },
                Final { &test1Sub2 },
                Transition { OnDone, Target { &test2 } }
            },
            State { &test2,
                OnEntry { [=] {
                              printf("Inside Test2\n");
                              qApp->exit(0);
                          } }
            }
        }

    );
}

}
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QStateMachine* sm = doIt();
    sm->start();
    QTimer::singleShot(0, [=]{
        raise<Event1>(sm);
    });
    return a.exec();
}
