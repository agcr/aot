// ==========  This file is under  LGPL, the GNU Lesser General Public Licence
// ==========  Dialing Structural Dictionary (www.aot.ru)
// ==========  Copyright by Alexey Sokirko (1998-2002)

#pragma once 

#include "StructEntry.h"
#include "ItemsContainer.h"
#include "CortegeContainer.h"
#include "DictConfig.h"


#include "morph_dict/common/utilit.h"
#include <time.h>



const size_t UnitCommentSize = 100;
class TUnitComment {
public:
	  int  m_EntryId;
	  char Editor[AuthorNameSize];
	  char Comments[UnitCommentSize];
      tm   modif_tm;

	  TUnitComment();
	  TUnitComment(int _UnitId);

      bool operator==(const TUnitComment& X) const
			{return    m_EntryId == X.m_EntryId; }

      bool operator<(const TUnitComment& X) const
	  { return    m_EntryId < X.m_EntryId;}

     
};	  

class TRoss  : public TCortegeContainer, public TItemContainer  
{
protected:
	std::string UnitsFile;
	std::string CortegeFile;
	std::string UnitCommentsFile;
	TDictConfig Config;

	void BuildCorteges();

public:

	std::string					m_DictName;
	
	std::vector<CStructEntry>	m_Units;
	std::vector<TUnitComment>	m_UnitComments;
	bool					m_bShouldSaveComments;


	TRoss ();
	~TRoss ();
	void	LoadOnlyConstants(const char* _RossPath);
	bool	Save ();


	// ++++++++++++++++++++++++++++++++++++++++++++++++++++
	// ++++++++       Словарный вход  +++++++++++++++++++++
	// ++++++++++++++++++++++++++++++++++++++++++++++++++++

	void	BuildUnits();
	uint16_t	GetUnitsSize() const { return (uint16_t)m_Units.size();	};
	bool	ClearUnits();
	void	ClearUnit(uint16_t UnitNo);
	void	DelUnit(std::vector<CStructEntry>::iterator It);
	uint16_t	LocateUnit (const char * EntryStr, int MeanNum) const;
	uint16_t    GetSelectedUnitNo (uint16_t i) const;
	uint16_t	GetSelectedUnitsSize() const;
	uint16_t    InsertUnit (CStructEntry& T);
	uint16_t	InsertUnit(const char* EntryStr, BYTE MeanNum);
	uint16_t    UnitsLowerBound(const char* EntryStr)  const 
	{ 
		return lower_bound(m_Units.begin() , m_Units.end(), CStructEntry(EntryStr, 1)) - m_Units.begin();
	};


	// ++++++++++++++++++++++++++++++++++++++++++++++++++++
	// ++++++++       Комментарий ко входу +++++++++++++++++++++
	// ++++++++++++++++++++++++++++++++++++++++++++++++++++

	bool			ReadUnitComments();
	uint16_t			InsertUnitComment (uint16_t m_EntryId);
	TUnitComment*			GetCommentsByUnitId(uint16_t m_EntryId); 
	const TUnitComment*		GetCommentsByUnitId(uint16_t EntryId)   const;


	// ++++++++++++++++++++++++++++++++++++++++++++++++++++
	// ++++++++       Словарная статья в виде кортежей ++++
	// ++++++++++++++++++++++++++++++++++++++++++++++++++++

	void			DelCorteges (size_t start, size_t last);
						 
	// ++++++++++++++++++++++++++++++++++++++++++++++++++++
	// ++++++++  Чтение/запись строки словарной статьи +++++
	// ++++++++++++++++++++++++++++++++++++++++++++++++++++


	bool	ReadFromStrWithOneSignatura (const char* s, TCortege& C, const CSignat& Sgn);
	bool	ReadFromStr (const char* s, TCortege& C);
	std::string	WriteToString (const TCortege& C) const;
	const CSignat& GetSignat(const TCortege& C) const;

	// ++++++++++++++++++++++++++++++++++++++++++++++++++++
	// +++++++++++++      Перечни констант  ++++++++++++++
	// ++++++++++++++++++++++++++++++++++++++++++++++++++++

	dom_item_id_t	InsertDomItem (const char* ItemStr, BYTE DomNo);

	const char* GetTitleFieldName() const;
	const char* GetSenseFieldName() const;
	const char* GetCommFieldName() const;
	const char* GetAuthorFieldName() const;
	const char* GetTimeCreatFieldName() const;
    const char* GetRedactFieldName() const;


	void SetUnitCommentStr(uint16_t UnitNo, const char* Str);
	void SetUnitAuthor(uint16_t UnitNo, const char* Author);
	void SetUnitModifTimeStr(uint16_t UnitNo, const char* TimeStr);
	void SetUnitEditor(uint16_t UnitNo, const char* Editor);
	std::string GetUnitModifTimeStr(uint16_t UnitNo) const;
	std::string GetUnitTextHeader(uint16_t UnitNo) const;

	

};


struct CSourceLine 
{
	std::string m_Line;
	int m_SourceLineNo;
	CSourceLine (std::string Line,  int  SourceLineNo)
	{
		m_Line = Line;
	    m_SourceLineNo = SourceLineNo;
	};
};



class CDictionary : public TRoss
{
	void		ProcessOneArticle(std::vector<CSourceLine>& L);

public:
	
	std::vector<CStructEntry>& GetUnits();

	CDictionary();
	
	bool		IsEmptyArticle(uint16_t UnitNo) const;
	int			GetUnitStartPos(uint16_t UnitNo) const;
	int			GetUnitEndPos(uint16_t UnitNo) const;
	BYTE		GetFieldNoByFieldStr(const char* Str) const;

	const std::string& GetDomItemStr(dom_item_id_t item_id) const;
	const std::string& GetDomItemStr(long CortegeNo, BYTE PositionInCortege) const;

	

	std::string	GetEntryStr (uint16_t EntryNo) const;
	BYTE		GetUnitMeanNum(uint16_t EntryNo) const;
	bool		IncludeArticle(uint16_t UnitNo, std::string Article) const;

	void		Load(const char* Path);
	void		SetUnitCurrentTime(uint16_t UnitNo);
	std::string		GetUnitEditor(uint16_t UnitNo) const;
	void		SetUnitStr(uint16_t UnitNo, const char*  UnitStr);
	void LoadAndExportDict(std::string fileName, std::string folder);
	void ImportFromTextFile(std::string fileName, std::string folder);

};	
