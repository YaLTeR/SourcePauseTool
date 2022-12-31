#include "..\feature.hpp"

// Feature description
class SaveloadsFeature : public FeatureWrapper<SaveloadsFeature>
{
public:
	void Begin(const char* segName_, int startIndex_, int endIndex_, int ticksToWait_, const char* extra); 
	void Stop();

protected:
	virtual bool ShouldLoadFeature() override;
	virtual void InitHooks() override;
	virtual void LoadFeature() override;
	//virtual void UnloadFeature() override;

private:
	std::string prefixName;
	int startIndex;
	int endIndex;
	int ticksToWait;

	int GetSignOnState();
	uintptr_t ORIG_SignOnState;
	std::vector<patterns::MatchedPattern> signOnStateMatches;
	int lastSignOnState;

	std::string extraCommands;
	bool execute = false;
	void Update();
};

static SaveloadsFeature spt_saveloads;
