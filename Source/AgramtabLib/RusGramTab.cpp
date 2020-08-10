﻿#include "StdGramtab.h"

// ==========  This file is under  LGPL, the GNU Lesser General Public Licence
// ==========  Dialing Lemmatizer (www.aot.ru)
// ==========  Copyright by Alexey Sokirko

#include "RusGramTab.h"


// не должно быть пробелов между словами 

const char* rPartOfSpeechesStr[] =
{ "С",  // 0
	"П", // 1
	"Г", // 2
	"МС", // 3
	"МС-П", // 4
	"МС-ПРЕДК", // 5
	"ЧИСЛ", // 6
	"ЧИСЛ-П", // 7
	"Н", // 8
	"ПРЕДК", //9 
	"ПРЕДЛ", // 10
	"ПОСЛ", // 11
	"СОЮЗ", // 12
	"МЕЖД", // 13
	"ВВОДН",// 14
	"ФРАЗ", // 15
	"ЧАСТ", // 16
	"КР_ПРИЛ",  // 17
	"ПРИЧАСТИЕ", //18
	"ДЕЕПРИЧАСТИЕ", //19
	"КР_ПРИЧАСТИЕ", // 20
	"ИНФИНИТИВ"  //21
};

const short GrammemsCount = 52;
const char* Grammems[GrammemsCount] = {
	// 0..1
	"мн","ед",
	// 2..8
	"им","рд","дт","вн","тв","пр","зв",
	// род 9-12
	"мр","жр","ср","мр-жр",
	// 13..15
	"нст","буд","прш",
	// 16..18
	"1л","2л","3л",
	// 19
	"пвл",
	// 20..21
	"од","но",
	// 22
	"сравн",
	// 23..24
	"св","нс",
	// 25..26
	"нп","пе",
	// 27..28
	"дст","стр",
	// 29-31
	"0", "аббр", "отч",
	// 32-33
	"лок", "орг",
	// 34-35
	"кач", "дфст",
	// 36-37 (наречия)
	"вопр", "указат",
	// 38..39
	"имя","фам",
	// 40
	"безл",
	// 41,42
	"жарг", "опч",
	// 43,44,45
	"разг", "притяж", "арх",
	// для второго родительного и второго предложного
	"2",
	"поэт", "проф",
	"прев", "полож"
};




const int rClauseTypesCount = 12;
const char rClauseTypes[rClauseTypesCount][30] =
{
	"ГЛ_ЛИЧН",
	"ДПР",
	"КР_ПРЧ",
	"КР_ПРИЛ",
	"ПРЕДК",
	"ПРЧ",
	"ИНФ",
	"ВВОД",
	"ТИРЕ",
	"НСО",
	"СРАВН",
	"КОПУЛ"
};




CRusGramTab::CRusGramTab()
{
	size_t i = 0;

	for (; i < GetMaxGrmCount(); i++)
		GetLine(i) = NULL;

}

CRusGramTab :: ~CRusGramTab()
{
	for (size_t i = 0; i < GetMaxGrmCount(); i++)
		if (GetLine(i) != NULL)
			delete GetLine(i);

}
const char* CRusGramTab::GetRegistryString() const
{
	return "Software\\Dialing\\Lemmatizer\\Russian\\Rgramtab";
};



BYTE	CRusGramTab::GetPartOfSpeechesCount() const
{
	return rPartOfSpeechCount;
};

size_t CRusGramTab::GetMaxGrmCount() const
{
	return MaxGrmCount;
}

const char* CRusGramTab::GetPartOfSpeechStr(BYTE i) const
{
	return rPartOfSpeechesStr[i];
};

size_t			CRusGramTab::GetGrammemsCount()  const
{
	return GrammemsCount;
};

const char* CRusGramTab::GetGrammemStr(size_t i) const
{
	return Grammems[i];
};

CAgramtabLine*& CRusGramTab::GetLine(size_t LineNo)
{
	return Lines[LineNo];
}

const CAgramtabLine* CRusGramTab::GetLine(size_t LineNo) const
{
	return Lines[LineNo];
}

size_t CRusGramTab::s2i(const char* s)  const
{
	return  (unsigned char)s[0] * 0x100 + (unsigned char)s[1] - StartUp;
};

std::string CRusGramTab::i2s(WORD i) const
{
	i += StartUp;
	char res[3];

	res[0] = (i >> 8);
	res[1] = (0xFF & i);
	res[2] = 0;
	return  res;
};



bool CRusGramTab::ProcessPOSAndGrammems(const char* tab_str, BYTE& PartOfSpeech, QWORD& grammems) const
{
	if (!CAgramtab::ProcessPOSAndGrammems(tab_str, PartOfSpeech, grammems)) return false;



	// неизменяемые слова как будто принадлежат всем падежам
	if ((_QM(rIndeclinable) & grammems)
		&& (PartOfSpeech != PREDK)
		)
		grammems |= rAllCases;

	if ((_QM(rIndeclinable) & grammems)
		&& (PartOfSpeech == PRONOUN_P)
		)
		grammems |= rAllGenders | rAllNumbers;


	// слова общего рода ('сирота') могут  использованы как 
	// слова м.р., так и как слова ж.р.
	if (_QM(rMascFem) & grammems)
		grammems |= _QM(rMasculinum) | _QM(rFeminum);

	// слово 'пальто' не изменяется по числам, поэтому может
	// быть использовано в обоих числах
	if (PartOfSpeech != PREDK)
		if ((_QM(rIndeclinable) & grammems) && !(_QM(rSingular) & grammems))
			grammems |= _QM(rPlural) | _QM(rSingular);

	return true;
};








// Стандартное согласование между двумя именами  по  падежу, причем первый код должен
// иметь граммему множественного числа 
inline bool CaseFirstPlural(const CAgramtabLine* l1, const CAgramtabLine* l2)
{
	return (((rAllCases & l1->m_Grammems & l2->m_Grammems) > 0)
		&& (((1 << rPlural) & l1->m_Grammems) > 0)
		);
};


// Стандартное согласование между двумя именами  по роду и числу 
inline bool GenderNumber(const CAgramtabLine* l1, const CAgramtabLine* l2)
{

	return 	((rAllNumbers & l1->m_Grammems & l2->m_Grammems) > 0)
		&& (((l1->m_Grammems & l2->m_Grammems & _QM(rPlural)) > 0)
			|| ((rAllGenders & l1->m_Grammems & l2->m_Grammems) > 0)
			);
}

// Стандартное согласование по лицу и числу (для 1 и 2 лица)
inline bool PersonNumber(const CAgramtabLine* l1, const CAgramtabLine* l2)
{
	size_t t1 = (rAllNumbers & l1->m_Grammems & l2->m_Grammems) > 0;
	size_t t2 = ((rAllPersons & l1->m_Grammems & l2->m_Grammems) > 0);
	return t1 && t2;
}

// Стандартное согласование между подлежащим и сказуемым
inline bool SubjectPredicate(const CAgramtabLine* subj_l, const CAgramtabLine* verb_l)
{
	const QWORD& subj = subj_l->m_Grammems;
	const QWORD& verb = verb_l->m_Grammems;

	if (!(subj & _QM(rNominativ))
		)
		return false;


	if (((verb & _QM(rPastTense)) > 0)
		|| (verb_l->m_PartOfSpeech == ADJ_SHORT)
		|| (verb_l->m_PartOfSpeech == PARTICIPLE_SHORT)
		)
	{
		// ты вышел 
		// я вышел
		// ты был
		// я красива
		// мы шли
		// вы шли
		if (subj & (_QM(rFirstPerson) | _QM(rSecondPerson)))
			return		(verb & subj & _QM(rPlural))
			|| ((verb & (_QM(rMasculinum) | _QM(rFeminum)))
				&& (verb & subj & _QM(rSingular))
				);
		else
			// он вышел
			// поезд ушел
			// девочка красива
			// девочки красивы 	
			// мальчик красив
			return GenderNumber(subj_l, verb_l);
	}
	else
		if ((verb & _QM(rPresentTense))
			|| (verb & _QM(rFutureTense)))
		{
			//  я выйду
			//  ты выедешь
			//  мы выйдем
			if ((subj & (_QM(rFirstPerson) | _QM(rSecondPerson)))
				|| (verb & (_QM(rFirstPerson) | _QM(rSecondPerson))))
				return PersonNumber(subj_l, verb_l);
			else
				return (rAllNumbers & subj & verb) > 0;
		}
		else
			if (verb & _QM(rImperative))
				return		((subj & _QM(rSecondPerson)) > 0)
				&& ((rAllNumbers & subj & verb) > 0);

	return false;
}


// Стандартное согласование между двумя именами  по  падежу 
inline bool Case(const CAgramtabLine* l1, const CAgramtabLine* l2)
{

	return 	((rAllCases & l1->m_Grammems & l2->m_Grammems) > 0);
}


// Стандартное согласование между существительным и прилашательнымпо роду, числу и падежу, если 
// если существительное одушевленное
bool GenderNumberCaseAnimRussian(const CAgramtabLine* l1, const CAgramtabLine* l2)
{
	const QWORD& gram_noun = l1->m_Grammems;
	const QWORD& gram_adj = l2->m_Grammems;

	return			((rAllCases & gram_noun & gram_adj) > 0)
		&& ((rAllNumbers & gram_noun & gram_adj) > 0)
		&& (((_QM(rAnimative) & gram_adj) > 0)
			|| ((rAllAnimative & gram_adj) == 0)
			)
		&& (((rAllGenders & gram_noun & gram_adj) > 0)
			|| ((rAllGenders & gram_noun) == 0)
			|| ((rAllGenders & gram_adj) == 0)
			);
};

// Стандартное согласование между существительным и прилашательнымпо роду, числу и падежу, если 
// если существительное неодушевленное
bool GenderNumberCaseNotAnimRussian(const CAgramtabLine* l1, const CAgramtabLine* l2)
{
	const QWORD& gram_noun = l1->m_Grammems;
	const QWORD& gram_adj = l2->m_Grammems;

	return			((rAllCases & gram_noun & gram_adj) > 0)
		&& ((rAllNumbers & gram_noun & gram_adj) > 0)
		&& (((_QM(rNonAnimative) & gram_adj) > 0)
			|| ((rAllAnimative & gram_adj) == 0)
			)
		&& (((rAllGenders & gram_noun & gram_adj) > 0)
			|| ((rAllGenders & gram_noun) == 0)
			|| ((rAllGenders & gram_adj) == 0)
			);
};

// Стандартное согласование между существительным и прилашательнымпо роду, числу и падежу, если 
// если существительное не неодушевленное и не одушевленное
//  (для местоимений, например, "все это было хорошо")
bool GenderNumberCaseRussian(const CAgramtabLine* l1, const CAgramtabLine* l2)
{
	const QWORD& gram_noun = l1->m_Grammems;
	const QWORD& gram_adj = l2->m_Grammems;

	return			((rAllCases & gram_noun & gram_adj) > 0)
		&& ((rAllNumbers & gram_noun & gram_adj) > 0)
		&& (((rAllGenders & gram_noun & gram_adj) > 0)
			|| ((rAllGenders & gram_noun) == 0)
			|| ((rAllGenders & gram_adj) == 0)
			);
};

bool FiniteFormCoordRussian(const CAgramtabLine* l1, const CAgramtabLine* l2)
{
	const QWORD& verb1 = l1->m_Grammems;
	const QWORD& verb2 = l2->m_Grammems;

	// жил и был
	if (verb1 & verb2 & rAllNumbers)
		if (verb1 & verb2 & _QM(rPastTense))
		{

			if ((verb1 & verb2 & rAllGenders)
				|| ((verb1 & verb2 & rAllNumbers) == _QM(rPlural))
				)
				return true;
		}
		else
			if (verb1 & verb2 & rAllPersons)
				return true;

	return false;

};




/*
Примеры работы функции GleicheGenderNumberCase:
- красивого стол;
- красивых столы;
- красивый пирата;
- красивые пиратов;
- достойных жизни;
- достойные матерей;
- длинные чудовищ
+ на этот дом;
+ красивую маму;
+ в красивый  дом;
+ в длинное облако;
+ примерить его цилиндр;
+ все это
*/
QWORD CRusGramTab::GleicheGenderNumberCase(const char* common_gram_code_noun, const char* gram_code_noun, const char* gram_code_adj) const
{
	if ((common_gram_code_noun == 0)
		|| !strcmp(common_gram_code_noun, "??")
		|| (strlen(common_gram_code_noun) == 0)
		)
		// нет сведений об одушевленности
		return Gleiche(GenderNumberCaseRussian, gram_code_noun, gram_code_adj);
	else
		if ((GetLine(s2i(common_gram_code_noun))->m_Grammems & _QM(rNonAnimative)) > 0)
			// неодушевленный
			return Gleiche(GenderNumberCaseNotAnimRussian, gram_code_noun, gram_code_adj);
		else
			if ((GetLine(s2i(common_gram_code_noun))->m_Grammems & _QM(rAnimative)) > 0)
				// одушевленный
				return Gleiche(GenderNumberCaseAnimRussian, gram_code_noun, gram_code_adj);
			else
				// нет сведений об одушевленности
				return Gleiche(GenderNumberCaseRussian, gram_code_noun, gram_code_adj);

}



bool CRusGramTab::GleicheCaseNumber(const char* gram_code1, const char* gram_code2) const
{
	return  Gleiche(CaseNumber, gram_code1, gram_code2) != 0;
}
bool CRusGramTab::GleicheGenderNumber(const char* gram_code1, const char* gram_code2) const
{
	return  Gleiche(GenderNumber, gram_code1, gram_code2) != 0;
}
//with absent grammems check, less strict than GleicheGenderNumber
bool CRusGramTab::ConflictGenderNumber(const char* gram_code1, const char* gram_code2) const
{
	return  Gleiche(GenderNumber0, gram_code1, gram_code2) == 0;
}
bool CRusGramTab::ConflictGrammems(QWORD g1, QWORD g2, QWORD breaks) const
{
	QWORD BR[] = { rAllCases, rAllNumbers, rAllGenders };
	bool R = true;
	for (int i = 0; i < (sizeof BR) / (sizeof BR[0]) && R; i++)
	{
		if (breaks & BR[i])
			R &= ((BR[i] & g1 & g2) > 0 || !(BR[i] & g1) || !(BR[i] & g2));
	}
	return  !R;
}
bool CRusGramTab::GleicheSubjectPredicate(const char* gram_code1, const char* gram_code2) const
{
	return  Gleiche(SubjectPredicate, gram_code1, gram_code2) != 0;
}

bool CRusGramTab::GleicheCase(const char* gram_code1, const char* gram_code2) const
{
	return  Gleiche(Case, gram_code1, gram_code2) != 0;
}


const size_t CRusGramTab::GetClauseTypesCount() const
{
	return rClauseTypesCount;
};

long CRusGramTab::GetClauseTypeByName(const char* TypeName) const
{
	for (int i = 0; i < rClauseTypesCount; i++)
	{
		if (!strcmp(rClauseTypes[i], TypeName))
		{
			return i;
		}
	}

	return -1;
}


const char* CRusGramTab::GetClauseNameByType(long type) const
{
	if (type >= rClauseTypesCount) return 0;
	if (type < 0) return 0;

	return rClauseTypes[type];
}










/*
	истина для предикативных типов клауз.
*/
bool CRusGramTab::IsStrongClauseRoot(const poses_mask_t poses) const
{
	return		(poses & (1 << VERB))
		|| (poses & (1 << ADVERB_PARTICIPLE)) // субъект деепричастия  совпадлает с субъектом
											 // главной  клаузы 
		|| (poses & (1 << PARTICIPLE_SHORT))
		|| (poses & (1 << ADJ_SHORT))
		|| (poses & (1 << PREDK));
};





const char* months[12] = { "ЯНВАРЬ","ФЕВРАЛЬ","МАРТ",
			   "АПРЕЛЬ","МАЙ","ИЮНЬ",
			   "ИЮЛЬ","АВГУСТ","СЕНТЯБРЬ"
			   ,"ОКТЯБРЬ","НОЯБРЬ","ДЕКАБРЬ" };

bool CRusGramTab::is_month(const char* lemma) const
{
	if (!lemma) return false;
	for (size_t i = 0; i < 12; i++)
		if (!strcmp(lemma, months[i]))
			return true;
	return false;
}

const char* SmallNumbers[5] = { "ДВА","ТРИ","ЧЕТЫРЕ","ОБА","ПОЛТОРА" };

bool CRusGramTab::is_small_number(const char* lemma) const
{
	if (!lemma) return false;
	for (size_t i = 0; i < 5; i++)
		if (!strcmp(lemma, SmallNumbers[i]))
			return true;
	return false;
}


bool CRusGramTab::IsMorphNoun(poses_mask_t poses)  const
{
	return  (poses & (1 << NOUN));
};




bool CRusGramTab::is_morph_adj(poses_mask_t poses) const
{
	return		(poses & (1 << ADJ_FULL))
		|| (poses & (1 << ADJ_SHORT));
};

bool CRusGramTab::is_morph_participle(poses_mask_t poses) const
{
	return  (poses & (1 << PARTICIPLE))
		|| (poses & (1 << PARTICIPLE_SHORT));
};

bool CRusGramTab::is_morph_pronoun(poses_mask_t poses) const
{
	return  (poses & (1 << PRONOUN)) != 0;
};


bool CRusGramTab::is_morph_pronoun_adjective(poses_mask_t poses) const
{
	return  (poses & (1 << PRONOUN_P)) != 0;
};


bool CRusGramTab::is_left_noun_modifier(poses_mask_t poses, QWORD grammems) const
{
	return     (poses & (1 << ADJ_FULL))
		|| (poses & (1 << NUMERAL_P))
		|| (poses & (1 << PRONOUN_P))
		|| (poses & (1 << PARTICIPLE));
}


bool CRusGramTab::is_numeral(poses_mask_t poses) const
{
	return  (poses & (1 << NUMERAL)) != 0;
};

bool CRusGramTab::is_verb_form(poses_mask_t poses) const
{
	return     is_morph_participle(poses)
		|| (poses & (1 << VERB))
		|| (poses & (1 << INFINITIVE))
		|| (poses & (1 << ADVERB_PARTICIPLE));
};



bool CRusGramTab::is_infinitive(poses_mask_t poses) const
{
	return (poses & (1 << INFINITIVE)) != 0;
}

bool CRusGramTab::is_morph_predk(poses_mask_t poses) const
{
	return (poses & (1 << PREDK)) ||
		(poses & (1 << PRONOUN_PREDK));
}

bool CRusGramTab::is_morph_adv(poses_mask_t poses) const
{
	return (poses & (1 << ADV)) != 0;
}


bool CRusGramTab::is_morph_personal_pronoun(poses_mask_t poses, QWORD grammems) const
{
	return		 (poses & (1 << PRONOUN)) != 0
		&& (grammems & (_QM(rFirstPerson) | _QM(rSecondPerson) | _QM(rThirdPerson)));
};



const size_t  ParticleCount = 8;
const char Particles[ParticleCount][20] = { "ЛИ","ЖЕ","БЫ","УЖ","ТОЛЬКО", "Ж", "Б", "ЛЬ" };

bool CRusGramTab::IsSimpleParticle(const char* lemma, poses_mask_t poses) const
{
	if (!lemma) return false;
	if (!(poses & (1 << PARTICLE))) return false;
	for (int i = 0; i < ParticleCount; i++)
		if (!strcmp(lemma, Particles[i]))
			return true;

	return false;
}









/*
	Есть такие  мест. прилагательные, которые могут выступать в роли существительных:
	"КАЖДЫЙ", "ОДИН", "ДРУГОЙ","ТОТ","КОТОРЫЙ".
	Для  них не имеет смысла вводит аналогичные местоимения,поскольку  все они могут
	быть использованы во всех родах. Эти прилагательные могут вести себя так же, как
	существительные. Например,
	Я купил два яблока, в каждом оказалось много червяков
	Я знаю ту, которая тебя видит
	Один пришел, другой ушел.
	Я вижу дом, который разрушился.
	Вышеуказанные мест. прилагательные отличаются, например, от местоимения "этот" и "всякий", поскольку
	трудно себе представить что-то вроде:
	"эта пришла"
	"всякая пришла"
	Но возможно:
	"всякое бывает"
	"это бывает"
	Здесь "всякое" и "это" - простые местоимения ср рода.

	!! Таким образом, я не хочу дублировать в морфологии МС-П, если они могут использованы
	!! во всех родах в качестве существительного.

	1 марта 2001 года, Сокирко
*/
bool CRusGramTab::IsSynNoun(poses_mask_t poses, const char* Lemma) const
{
	return   IsMorphNoun(poses)
		|| (poses & (1 << PRONOUN))
		|| ((poses & (1 << PRONOUN_P))
			&& (!strcmp(Lemma, "КАЖДЫЙ")
				|| !strcmp(Lemma, "ОДИН")
				|| !strcmp(Lemma, "ДРУГОЙ")
				|| !strcmp(Lemma, "ТОТ")
				|| !strcmp(Lemma, "КОТОРЫЙ")
				)
			);
};


const int StandardParamAbbrCount = 8;
const char* StandardParamAbbr[StandardParamAbbrCount] =
{ "КГ", "КМ", "СМ","МЛ", "МБ","КБ", "МГЦ", "КВ" };

bool CRusGramTab::IsStandardParamAbbr(const char* WordStrUpper) const
{
	if (strlen(WordStrUpper) > 4) return false;
	for (long i = 0; i < StandardParamAbbrCount; i++)
		if (!strcmp(WordStrUpper, StandardParamAbbr[i]))
			return true;

	return false;
}


bool CRusGramTab::is_morph_article(poses_mask_t poses)  const
{
	return  false;
};

bool CRusGramTab::FilterNounNumeral(std::string& gcNoun, const std::string& gcNum, QWORD& grammems) const
{
	if (gcNoun.length() == 2 || !(grammems & rAllCases)) return false;
	GleicheAncode1(CaseNumberGender0, gcNum,
		FilterGramCodes(gcNum, grammems & rAllCases | ~rAllCases, 0), gcNoun);
	return true;
}


std::string RussianCaseNumberGender(const CAgramtab* pGramTab, const std::string& adj, const std::string& common_noun_grm, const std::string& noun)
{
	assert(false);
	//return pGramTab->GleicheGenderNumberCase(common_noun_grm.c_str(), noun.c_str(), adj.c_str());
	return "";
};

QWORD CRusGramTab::ChangeGleicheAncode1(GrammemCompare CompareFunc, const std::string& wordGramCodes, std::string& groupGramCodes, const QWORD wordGrammems) const
{
	groupGramCodes = GleicheAncode1(CompareFunc, FilterGramCodes(wordGramCodes, ~_QM(rIndeclinable), 0), groupGramCodes);
	if (groupGramCodes == "") { return 0; }
	const QWORD gramFilter = rAllCases | rAllGenders | rAllTimes | rAllPersons | rAllAnimative;
	return 	wordGrammems & ~(gramFilter) | GetAllGrammems(groupGramCodes.c_str());
}

