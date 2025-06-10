#include <stdbool.h>
#include <sys/queue.h>

#ifndef __YUGIOH__RULES__
#define __YUGIOH__RULES__

#define TURN_TIMER 480 // Seconds

enum YGOPhase {
	DRAW,
	STANDBY,
	MAIN,
	COMBAT,
	SECOND,
	END_TURN,
};

enum YGOCardType {
  MONSTER,
  MAGIC_TRAP,
};

struct YGOMagicCard {
	bool quick;
	bool continuous;
	bool equip;
	bool counter;
	bool field;
	bool ritual;
};

struct YGOMonsterCard {
	int atk;
	int def;
	int level;
};

struct YGOCard {
	enum YGOCardType type;
	char* name;
	char* desc;

	union {
		struct YGOMonsterCard monster;
		struct YGOMagicCard magic;
	} card;
	
	void (**effect)();
	void (**special_summon)();
};

struct YGOField {
	LIST_HEAD(, YGOCard) monsters_fields[2];
	LIST_HEAD(, YGOCard) spell_fields[2];
	LIST_HEAD(, YGOCard) graveyards[2];
	LIST_HEAD(, YGOCard) banlist[2];
	LIST_HEAD(, YGOCard) extradeck[2];
	struct YGOCard terrain[2];
};

struct YGO_Player {
	int life;
	char* name;
	LIST_HEAD(,YGOCard) cards;
};

#endif
