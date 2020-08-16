// ==========  This file is under  LGPL, the GNU Lesser General Public Licence
// ==========  Dialing Syntax Analysis (www.aot.ru)
// ==========  Copyright by Dmitry Pankratov, Igor Nozhov, Alexey Sokirko

#include  "StdSynan.h"
#include  "RusSyntaxOpt.h"
#include  "RusSentence.h"
#include  "RusThesaurus.h"
#include  "RusOborot.h"
#include  "../StructDictLib/TempArticle.h"


const int rSyntaxGroupTypesCount = 48;
const std::string rSyntaxGroupTypes [rSyntaxGroupTypesCount] = 
{
						_R("ПРИЛ_СУЩ"),				_R("КОЛИЧ"),      
_R("СЛОЖ_ЧИСЛ"),		_R("НАР_ЧИСЛ_СУЩ"),			_R("СУЩ_ЧИСЛ"),    
_R("ПГ"),				_R("ОДНОР_ПРИЛ"),			_R("НАРЕЧ_ГЛАГОЛ"),	
_R("НАР_ПРИЛ"),			_R("НАР_НАР"),				_R("СРАВН_СТЕПЕНЬ"),	
_R("ОТР_ФОРМА"),		_R("ПРЯМ_ДОП"),				_R("ГЕНИТ_ИГ"),			
_R("ОДНОР_НАР"),		_R("ПЕР_ГЛАГ_ИНФ"),			_R("ОДНОР_ИНФ"),			     
_R("ФИО"),				_R("ОДНОР_ИГ"),				_R("ЧИСЛ_СУЩ"),				
_R("ИГ"),				_R("ЭЛЕКТ_ИГ"),				_R("ЭЛ_АДРЕС"),
_R("ОДНОР_ЧИСЛ"),		_R("МОДИФ_ПРИЛ"),			_R("АППРОКС_ИГ"),
_R("НАР_НАР_ЧИСЛ"),		_R("АППРОКС_ПГ"),			_R("ПРИЧ_СУЩ"),
_R("ПОДЛ"),				_R("ОБОРОТ"),				_R("ПОДКЛАУЗА"),		
_R("ПРИЛОЖЕНИЕ"),		_R("СУЩ_ОБС_ПРИЛ"),			_R("КЛВ"),				
_R("РАЗРЫВ_СОЮЗ"),		_R("ОТСОЮЗ"),				_R("НАР_ПРЕДИК"),      
_R("ПРИЛ_ПОСТПОС"),     _R("ПРИДАТ_ОПР"),			_R("АНАТ_СРАВН"),      
_R("ВВОДН_КЛАУЗА"),     _R("ДОЛЖ_ФИО"),				_R("ИНОСТР_ЛЕКС"), 
_R("ОТСРАВН"),			_R("ПРЯМ_ДОП_РЕВ"),			_R("ИНСТР_ДОП")

};




CSyntaxOpt*  NewOptionsRussian ()
{
	return new CRusSyntaxOpt(morphRussian);
};

CRusSyntaxOpt :: CRusSyntaxOpt (MorphLanguageEnum langua) : CSyntaxOpt(langua)
{	
	SynDependOnAdv = NULL;
	SynDependOnAdj = NULL;
	m_pCompAdvList = NULL;
	m_pNounNumList = NULL;
	m_pVerbsWithInstrObj = NULL;
	m_pNumberAdverbsList = NULL;
    
	AdvAdj = NULL;
	VerbsThatCanSubdueInfinitive = NULL;
	
	m_lPradigmID_NECHEGO = -1;	

	m_SimilarNPGroupType = SIMILAR_NOUN_GROUPS;
	m_PluralMask = _QM(rPlural);
	m_SingularMask = _QM(rSingular);
	m_IndeclinableMask = _QM(rIndeclinable);
	m_GenNounGroupType = GEN_NOUN;
	m_PrepNounGroupType = PREP_NOUN;
	m_DirObjGroupType = DIR_OBJ;
	m_NPGroupType = NOUN_GR;
	m_NounAdjGroupType= NOUN_ADJUNCT;
	m_NameGroupType = NAMES;
	m_DisruptConjRelation = DISRUPT_CONJ_RELATION;
	m_DisruptConjGroupType = DISRUPT_CONJ;
	m_OborotGroupType = OBOROTS;
	m_WWWGroupType = WEB_ADDR;
	m_KEYBGroupType = KEYB;
	m_SubjRelation = SUBJ;
	m_RusParenthesis = INP;
	m_Preposition = PREP;
	m_Conjunction = CONJ;
	m_InfintiveClauseType = INFINITIVE_T;

	m_SyntaxGroupTypes.clear();
	for (size_t i=0; i < rSyntaxGroupTypesCount; i++)
		m_SyntaxGroupTypes.push_back(rSyntaxGroupTypes[i]);
	
}

CSentence* CRusSyntaxOpt::NewSentence() const {
	return new CRusSentence(this);
};

CAgramtab *CRusSyntaxOpt::NewGramTab() const {
	return new CRusGramTab();
};

CLemmatizer *CRusSyntaxOpt::NewLemmatizer() const {
	return new CLemmatizerRussian();
};

COborDic * CRusSyntaxOpt::NewOborDic(const CSyntaxOpt* opt)  {
	return new CRusOborDic(opt);
};

CThesaurusForSyntax* CRusSyntaxOpt::NewThesaurus(const CSyntaxOpt* opt) {
	return new CRusThesaurusForSyntax(opt);
};

void CRusSyntaxOpt::DestroyOptions ()
{
	CSyntaxOpt::DestroyOptions();
	if(m_pCompAdvList)
		delete m_pCompAdvList;
	if(m_pNounNumList)
		delete m_pNounNumList;
	if(m_pVerbsWithInstrObj)
		delete m_pVerbsWithInstrObj;
	
	if(m_pNumberAdverbsList)
		delete m_pNumberAdverbsList;
	if(AdvAdj)
		delete AdvAdj;
	if(SynDependOnAdj)
		delete SynDependOnAdj;
	if(SynDependOnAdv)
		delete SynDependOnAdv;
	if (VerbsThatCanSubdueInfinitive)
		delete VerbsThatCanSubdueInfinitive;

	
};





void BuildArticle(CDictionary* piRossDict, std::string s, CTempArticle& A1)
{
	A1.m_pRoss =  piRossDict;
	A1.SetArticleStr( s.c_str() );
	A1.MarkUp();
	A1.BuildCortegeList();
};


void CRusSyntaxOpt :: LoadFromRoss(CDictionary* piRossDict)
{
	int i;

	try
	{
		int iSize = piRossDict->GetUnitsSize();

		AdvAdj = new SDatItems(_QM(ADV));	
		SynDependOnAdv = new SDatItems(_QM(ADV));	
		SynDependOnAdj = new SDatItems(_QM(ADV));	

		CTempArticle A1;
		BuildArticle(piRossDict,_R("GF = * НАР:нар_опр"), A1);
		
		CTempArticle A2;
		BuildArticle(piRossDict,_R("GF1 = * X!:НАР"), A2);

		CTempArticle A3;
		BuildArticle(piRossDict,_R("GF1 = * X!:ПРИЛ"), A3);		

		
		CTempArticle A;
		A.m_pRoss = piRossDict;

		for(i = 0 ; i < iSize ; i++ )
		{
			A.ReadFromDictionary (i, false, true);
			
			const std::string& DebugWord = piRossDict->m_Units[i].m_EntryStr;

			if (A1.IsPartOf(&A, true))
			{
				std::string dat_item = piRossDict->m_Units[i].m_EntryStr;
				AdvAdj->m_vectorDatItems.push_back(dat_item);
			}

			if (A2.IsPartOf(&A, true))
			{
				std::string  dat_item = piRossDict->m_Units[i].m_EntryStr;
				SynDependOnAdv->m_vectorDatItems.push_back(dat_item);
			}

			if (A3.IsPartOf(&A, true))
			{
				std::string  dat_item = piRossDict->m_Units[i].m_EntryStr;
				SynDependOnAdj->m_vectorDatItems.push_back(dat_item);
			}
		}
	}
	catch(...)
	{
		char strMsg[200];
		strcpy(strMsg, "Problems with ROSS, word : \"");
		strcat (strMsg, piRossDict->m_Units[i].m_EntryStr);
		strcat (strMsg, "\"");
		OutputErrorString(strMsg);
	}
	SynDependOnAdj->Sort();
	SynDependOnAdv->Sort();
	AdvAdj->Sort();


}


const char g_strRegRossDicPath[] = "Software\\Dialing\\Ross\\DictPath"; 

static std::string GetSyntaxFilePath()
{
	return GetRmlVariable()+"/Dicts/SynAn/";
};


bool CRusSyntaxOpt :: InitOptionsLanguageSpecific()
{
	
	//loading ross
	try
	{			
		CDictionary piRossDict;	
		std::string strPath = GetRegistryString( g_strRegRossDicPath );
		if (!piRossDict.Load(strPath.c_str())) return false;
		LoadFromRoss(&piRossDict);
	
	}
	catch(CExpc& )
	{
		char strMsg[400];
		sprintf(strMsg, "Failed to find registry entry %s", g_strRegRossDicPath);
		OutputErrorString("Clause Builder");
		return false;			
	}
	catch(...)
	{			
		OutputErrorString("Failed to load \"StructDict.dll\" ( Ross)");	
		return false;
	}

	

	
	std::string Path = GetSyntaxFilePath();
  	

	m_pCompAdvList = new StringVector;
	if (!ReadListFile (Path+"comp_adv.dat",*(m_pCompAdvList)))
		return false;
	
	m_pNounNumList = new StringVector;
	if (!ReadListFile (Path+"noun_num.dat",*(m_pNounNumList)))
		return false;


	m_pNumberAdverbsList = new StringVector;
	if (!ReadListFile (Path+"num_pr.dat",*(m_pNumberAdverbsList)))
		return false;

	
	VerbsThatCanSubdueInfinitive = new SDatItems(_QM(VERB) | _QM(INFINITIVE) | _QM(ADVERB_PARTICIPLE) | _QM(PARTICIPLE_SHORT) | _QM(PARTICIPLE));	
	if (!ReadListFile (Path+"verbs_with_inf.txt", VerbsThatCanSubdueInfinitive->m_vectorDatItems))
		return false;
	VerbsThatCanSubdueInfinitive->Sort();

	m_pVerbsWithInstrObj = new SDatItems(_QM(VERB) | _QM(INFINITIVE) | _QM(ADVERB_PARTICIPLE) | _QM(PARTICIPLE_SHORT) | _QM(PARTICIPLE));	
	if (!ReadListFile (Path+"verbs_with_inf.txt", m_pVerbsWithInstrObj->m_vectorDatItems))
		return false;
	m_pVerbsWithInstrObj->Sort();
	
    if (m_pProfessions->m_vectorDatItems.empty())
    {
        // read it from file if thesaurus is disabled
	    if (!ReadListFile (Path+"profes.txt", m_pProfessions->m_vectorDatItems))
		    return false;
	    m_pProfessions->Sort();
    }


	try
	{
		vector<CFormInfo> Paradigms;
		std::string h = _R("нечего");
		GetLemmatizer()->CreateParadigmCollection(true, h, false, false, Paradigms);

		for (long k=0; k < Paradigms.size(); k++)
		{
		  std::string AnCode = Paradigms[k].GetAncode(0);
		  BYTE POS = GetGramTab()->GetPartOfSpeech(AnCode.c_str() );
		  if  (POS == PRONOUN_PREDK)
		  {
			m_lPradigmID_NECHEGO = Paradigms[k].GetParadigmId();
			m_Gramcode_NECHEGO = Paradigms[k].GetAncode(0);
			break;
		  };
		};


	}
	catch (...)
	{

	}


	return true;
}


bool CRusSyntaxOpt::is_firm_group(int GroupType) const
{
	return (		(GroupType == WEB_ADDR ) 
				||	(GroupType == NUMERALS ) 
				||	(GroupType == C_NUMERALS ) 
				||	(GroupType == NAMES )	
				||	(GroupType == KEYB )  
				||	(GroupType == FOREIGN_TERM )
				||	(GroupType == OBOROTS ) 
			);
}


bool CRusSyntaxOpt::IsGroupWithoutWeight(int GroupType, const char* cause) const
{
	//	группу ПРЯМ_ДОП не будем учитывать при подсчете веса, поскольку 
	//	иначе на след. примерах синтаксис работает неверно:
	//		СПС и "Яблоко" не дали Немцову побыть акционером "Эха"
	//	для фразы "Я считал его великим учителем." специльно 
	//	прописывается  частота омонима "его".

	
	if (GroupType == DIR_OBJ) return true;
	
	//  Если это ПГ, у которого главное слово - прилагательное, тогда 
	//  нужно взять ее первую подгруппу (если она есть) и считать ее вес
	//  например: "у самого ленивого" - здесь будет считаться  вес группы 
	//  "самого ленивого"
	
	if (    cause
			&& !strcmp(cause,"PP + adjective")
		)
		return true;


	//	группу ОТР_ФОРМА не будем учитывать при подсчете веса, поскольку 
	//	иначе на след. примерах синтаксис работает неверно:
	//		"не дело тебе ходить в лес"
	if (GroupType == NEG_VERB) return true;


	// пример для группы НАР_ГЛАГ "Было приятно ощущать тепло разогретого солнцем камня."
	if (GroupType == ADV_VERB) return  true;

	// "Куплю масла"
	if (GroupType == GEN_NOUN) return  true;


	// "Как он раньше жил?"
	if (GroupType == FROM_COMPAR) return  true;

	// пока не знаю примеров для:
	//if (GroupType == INSTR_OBJ) return  true;

	// "Серый рваный дым цеплялся за ветви елей , прижатый токами воздуха"
	if (GroupType == NOUN_PARTICIPLE) return  true;
	
	return false;

};

bool CRusSyntaxOpt::IsSimilarGroup (int type) const
{
	return   (type  == SIMILAR_ADJS)
		   ||(type  == SIMILAR_NOUN_GROUPS)
		   ||(type  == SIMILAR_INFS)
		   ||(type  == SIMILAR_ADVS)
		   ||(type  == SIMILAR_NUMERALS) ;
};
