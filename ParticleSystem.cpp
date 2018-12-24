
// Particle Systems create a sandbox of potential particles to use, which follow a custom function.
// This function is dependent on the current spacetime, and a set of random constants. (Particles are deterministic)
// Upon a render, the particles are organized in a BSP and rendered using the painter's algorythm.
// This allows for transparent particles.

// The system tracks the number of alive particles, which assemble in a linked list.
// The start of the dead particles is also tracked.
// When a particle is created, the dead list advances one link, which is then initialized.
// When a particle dies, it is moved to the start of the dead list. The hole is then patched.
// When no more particles can be created, the dead pointer points to null.
// When no particles are alive, the root pointer is the dead pointer.

#include "Engine.h"

ParticleSystem::ParticleSystem()
{
	Alive = 0;
	Max = 100;
	Data = new Particle[Max];
	Dead = &Data;
	Root = Data;
	for(int i=0;i<Max-1;++i)
		Data[i].Next = &Data[i+1];
	Data[Max-1].Next = nullptr;
}

void ParticleSystem::New(Particle *In)
{
	if(Alive == Max) return;
	Alive++;
	Particle *Fresh = *Dead;
	Particle *Next = Fresh->Next;
	Dead = &Fresh->Next;
	memcpy(Fresh, In, sizeof(Particle));
	Fresh->Next = Next;
}

void ParticleSystem::Remove(Particle** Prev)
{
	if(Alive == 0) return;
	Alive--;
	Particle *Victim = (*Prev)->Next;
	(*Prev)->Next = Victim->Next;
	Victim->Next = *Dead;
	Dead = &Victim->Next;
}

ParticleSystem::~ParticleSystem()
{
	delete[] Data;
	Data = nullptr;
}
