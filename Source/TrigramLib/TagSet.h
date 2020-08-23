#ifndef TagSet_h
#define TagSet_h

#include "../common/util_classes.h"
class CAgramtab;


struct CTag 
{
	std::string			m_Name;
	std::vector<std::string>	m_Lemmas;
	BYTE			m_Pos;
	QWORD			m_Grammems;
	

	CTag();
	bool operator < (const CTag& T) const;
	bool IsEmptyTag() const;
};



class CTagSet
{
public:
	std::map<std::string, CTag>	m_Tags;
	CTagSet(void);
	~CTagSet(void);
	// build the default (part of speech) tag std::set
	bool			BuildDefaultTags(const CAgramtab* pAgramtab);
	// read the tag std::set from file
	bool			ReadTagSet(std::string FileName, const CAgramtab* pAgramtab);
	//bool			ReadMyStemTagSet(std::string FileName, const CAgramtab* pAgramtab);

	CTag			GetBestSuitedTag (const CTag& A) const;
	std::vector<CTag>	DecipherTagStr (std::string TagStr, const CAgramtab* pAgramtab) const;
	CTag			GetTagFromAncode(const CAgramtab* pAgramtab, const char* Ancode, QWORD CommonGrammems, const std::string& Lemma) const;
};


#endif
