
// ==========  This file is under  LGPL, the GNU Lesser General Public Licence
// ==========  Dialing Syntax Analysis (www.aot.ru)
// ==========  Copyright by Dmitry Pankratov, Igor Nozhov, Alexey Sokirko

#include "StdSynan.h"
#include "RusSentence.h"


bool	CRusSentence::IsInitialClauseType(EClauseType ClauseType) const
{
	return 	
			ClauseType  == VERB_PERS_T
		||	ClauseType  == ADVERB_PARTICIPLE_T
		||	ClauseType  == PARTICIPLE_SHORT_T	
		||	ClauseType  == ADJ_SHORT_T
		||	ClauseType  == PREDK_T
		||	ClauseType  == PARTICIPLE_T	
		||	ClauseType  == INFINITIVE_T	
		||	ClauseType	== INP_T;
};




bool CRusSentence::SetClauseBorderIfThereAreTwoPotentialPredicates(int FWrd, int LWrd)
{
	if (!m_bShouldUseTwoPotentialRule) return false;
	int j, iPredicCount, iVerb, iBe, nBe, nBeInf, nAnalytPredic;
	iPredicCount = iVerb = iBe = 0;
	nBe = nAnalytPredic = nBeInf = -1;
	for (int i = FWrd; i <= LWrd; i++)
	{	
	  bool bVerb = true;	
	  //проверяю, что все омонимы одной словоформы являются предикат 	
	  for (j = 0; j < m_Words[i].GetHomonymsCount(); j++)
	  {
		  const CSynHomonym& Hom = m_Words[i].GetSynHomonym(j);

		  if (!Hom.HasPos(VERB)) bVerb = false; 	

	      if (Hom.IsLemma("БЫТЬ") && Hom.HasPos(INFINITIVE) )
		  	nBeInf = i;

		  if ( GetOpt()->GetGramTab()->IsStrongClauseRoot(Hom.m_iPoses))
		  {
				//не проверяю глагол "стать", т.к. он имеет омоним сущ-ного 
				if (	Hom.IsLemma("БЫТЬ")
					&&!( Hom.HasGrammem(rPresentTense) && Hom.HasPos(VERB) ) )
				{
					iBe++;
					nBe = i;
					break;
				}
				//глаголы, которые не являются самостоятельным предикатом или суть ан.ф.
				if (!Hom.IsLemma("ДАВАТЬ")&&
					!Hom.IsLemma("СТАНОВИТЬСЯ") &&
					!Hom.IsLemma("ОКАЗАТЬСЯ")&&
					!Hom.IsLemma("СТАТЬ"))
					continue;

		  }
		  break; 	
	  }

	  if (j == m_Words[i].GetHomonymsCount())
	  {
		  iPredicCount++;
		  if (bVerb) 
		  {
			iVerb++;
			continue;
		  }
		  nAnalytPredic = i;
	  }
	}	
	
	assert(iPredicCount < 4);
	//случай, когда аналитическая форма может быть инфинитивом:
	//'может быть автоматизирован'
	CIntVector dummyVector;
	int DummyHomonymNo;
	if ( (2 == iPredicCount) && (-1 != nBeInf) && (-1 != nAnalytPredic) )
		if ( IsAnalyticalVerbForm(nBeInf, nAnalytPredic, DummyHomonymNo, dummyVector) )
			return false;

	//если два предиката в одной клаузе, восстанавливаю запятую	
	if	(		(2 <= iPredicCount) 
			||	(2 == iBe) 
			||	(1 == iVerb && 1 == iBe) 
			||	( 		
					//если вспомогательный глагол и предикат не образуют ан. ф., восстанавливаю запятую
						(1 == iPredicCount)
					&&	(1 == iBe)
					&&	(nBe != -1)
					&&	(nAnalytPredic != -1)
					&&	!IsAnalyticalVerbForm(nBe, nAnalytPredic, DummyHomonymNo, dummyVector)
				)
		) 
	{
		return true;
	}


	return false;	
};



