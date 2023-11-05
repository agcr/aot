// SetOfWordPairs.h : Declaration of the CSetOfWordPairs

#ifndef __SETOFWORDPAIRS_H_
#define __SETOFWORDPAIRS_H_

#include "morph_dict/common/utilit.h"


/////////////////////////////////////////////////////////////////////////////
// CSetOfWordPairs
class CBinaryDictionary;
class CSetOfWordPairs 
{
	int begin, end;
	const CBinaryDictionary *dict;
	bool is_direct;
	void SetRange(long _begin, long _end);
	void SetDictionary(CBinaryDictionary const *d, bool direct);
	friend class CBinaryDictionary;
public:

	CSetOfWordPairs();

	long GetId(long index) const;
	long get_Count() const;
    bool IsNormalLanguage(long pair_index) const;
    part_of_speech_t GetSimplifiedPartOfSpeech(long pair_index) const;
};

#endif //__SETOFWORDPAIRS_H_
