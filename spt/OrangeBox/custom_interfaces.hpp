#pragma once
#include "cdll_int.h"

#ifdef OE
class IVEngineClientDMoMM {
public:
	virtual void Unk1() = 0;
	virtual void Unk2() = 0;
	virtual void Unk3() = 0;
	virtual void Unk4() = 0;
	virtual void Unk5() = 0;
	virtual void Unk6() = 0;
	virtual void Unk7() = 0;
	virtual void Unk8() = 0;
	virtual void Unk9() = 0;
	virtual void ClientCmd(const char* szCmdString) = 0;
	virtual void Unk10() = 0;
	virtual void Unk11() = 0;
	virtual void Unk12() = 0;
	virtual void Unk13() = 0;
	virtual void Unk14() = 0;
	virtual void Unk15() = 0;
	virtual void Unk16() = 0;
	virtual void Unk17() = 0;
	virtual void Unk18() = 0;
	virtual void Unk19() = 0;
	virtual void Unk20() = 0;
	virtual void GetViewAngles(QAngle& va) = 0;
	virtual void SetViewAngles(QAngle& va) = 0;
	virtual void Unk21() = 0;
	virtual void Unk22() = 0;
	virtual void Unk23() = 0;
	virtual void Unk24() = 0;
	virtual void Unk25() = 0;
	virtual void Unk26() = 0;
	virtual void Unk27() = 0;
	virtual void Unk28() = 0;
	virtual void Unk29() = 0;
	virtual void Unk30() = 0;
	virtual void Unk31() = 0;
	virtual int Cmd_Argc() = 0;
	virtual const char* Cmd_Argv(int arg) = 0;

	// Others ommitted.
};
#endif

class EngineClientWrapper {
public:
	virtual ~EngineClientWrapper() {};
	virtual void ClientCmd(const char* command) = 0;
	virtual void GetViewAngles(QAngle& viewangles) = 0;
	virtual void SetViewAngles(QAngle& viewangles) = 0;

#ifdef OE
	virtual int Cmd_Argc() = 0;
	virtual const char* Cmd_Argv(int arg) = 0;
#else
	virtual const char* GetGameDirectory() = 0;
#endif
};

/**
 * Wrapper for an interface similar to IVEngineClient.
 */
template<class EngineClient>
class IVEngineClientWrapper : public EngineClientWrapper {
public:
	IVEngineClientWrapper(EngineClient* engine) : engine(engine) {}

	void ClientCmd(const char* command) override {
		return engine->ClientCmd(command);
	}

	void GetViewAngles(QAngle& viewangles) override {
		return engine->GetViewAngles(viewangles);
	}

	void SetViewAngles(QAngle& viewangles) override {
		return engine->SetViewAngles(viewangles);
	}

#ifdef OE
	int Cmd_Argc() override {
		return engine->Cmd_Argc();
	}

	const char* Cmd_Argv(int arg) override {
		return engine->Cmd_Argv(arg);
	}
#else
	const char* GetGameDirectory() override {
		return engine->GetGameDirectory();
	}
#endif

private:
	EngineClient* engine;
};
