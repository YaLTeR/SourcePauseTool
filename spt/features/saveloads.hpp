#include "..\feature.hpp"

// Feature that does automated save/loading operations, such as save/load segment creation or rendering
class SaveloadsFeature : public FeatureWrapper<SaveloadsFeature>
{
public:
	void Begin(int type_, const char* segName_, int startIndex_, int endIndex_, int ticksToWait_, const char* extra); 
	void Stop();

protected:
	virtual bool ShouldLoadFeature() override;
	virtual void LoadFeature() override;
	//virtual void UnloadFeature() override;

private:
	int type;
	std::string prefixName;
	int startIndex;
	int endIndex;
	int ticksToWait;
	int lastSignOnState;

	std::string extraCommands;
	bool execute = false;
	void Update();
};

static SaveloadsFeature spt_saveloads;
