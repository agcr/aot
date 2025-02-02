// ==========  This file is under  LGPL, the GNU Lesser General Public License
// ==========  Dialing Syntax Analysis (www.aot.ru)
// ==========  Copyright by Dmitry Pankratov, Igor Nozhov, Alexey Sokirko

#include "RusFormatCaller.h"
#include "../RusClause.h"


CRusFormatCaller::CRusFormatCaller(const CSyntaxOpt* Opt) : CFormatCaller(Opt)
{
	FormatCalls = {
		{rule_for_numbers, {(CFormatFunc)&CRusFormatCaller::format_for_numbers, "КОЛИЧ", FROM_LEFT}},
		{rule_for_numbers, {(CFormatFunc)&CRusFormatCaller::format_for_numbers, "КОЛИЧ", FROM_LEFT}},
		{rule_for_num_complex, {(CFormatFunc)&CRusFormatCaller::format_for_num_complex, "СЛОЖ_ЧИСЛ", FROM_LEFT}},
		{rule_for_similar_numerals, {(CFormatFunc)&CRusFormatCaller::format_for_similar_numerals, "ОДНОР_ЧИСЛ", FROM_LEFT}},
		{rule_for_noun_adjuncts, {(CFormatFunc)&CRusFormatCaller::format_for_noun_adjunct_group, "ПРИЛОЖЕНИЕ", FROM_LEFT}},
		{rule_for_noun_num, {(CFormatFunc)&CRusFormatCaller::format_for_noun_num, "СУЩ_ЧИСЛ", FROM_LEFT}},
		{rule_for_names, {(CFormatFunc)&CRusFormatCaller::format_for_FAM1_FAM2, "ФИО", FROM_LEFT}},
		{rule_for_adv_adj, {(CFormatFunc)&CRusFormatCaller::format_for_adv_adj,"НАР_ПРИЛ", FROM_LEFT}},
		{rule_for_similar_adjs, {(CFormatFunc)&CRusFormatCaller::format_for_similar_adjs,"ОДНОР_ПРИЛ", FROM_LEFT}},
		{rule_for_modif_adj, {(CFormatFunc)&CRusFormatCaller::format_for_modif_adj_groups,"МОДИФ_ПРИЛ", FROM_LEFT}},
		{rule_for_similar_advs, {(CFormatFunc)&CRusFormatCaller::format_for_similar_advs,"ОДНОР_НАР", FROM_LEFT}},
		{rule_for_similar_advs_comp_conj, {(CFormatFunc)&CRusFormatCaller::format_for_similar_advs_comp_conj,"ОДНОР_НАР", FROM_LEFT}},
		{rule_for_similar_infs, {(CFormatFunc)&CRusFormatCaller::format_for_similar_infs,"ОДНОР_ИНФ", FROM_LEFT}},
		{rule_for_similar_comp_adjs, {(CFormatFunc)&CRusFormatCaller::format_for_similar_comp_adjs,"ОДНОР_ПРИЛ", FROM_LEFT}},
		{rule_for_web_addr, {(CFormatFunc)&CRusFormatCaller::format_for_web_addr,"ЭЛ_АДРЕС", FROM_LEFT}},
		{rule_for_comp_adv, {(CFormatFunc)&CRusFormatCaller::format_for_comp_adv,"СРАВН_СТЕПЕНЬ", FROM_LEFT}},
		{rule_for_adv_verb, {(CFormatFunc)&CRusFormatCaller::format_for_adv_verb,"НАРЕЧ_ГЛАГОЛ", FROM_LEFT}},
		{rule_for_noun_groups, {(CFormatFunc)&CRusFormatCaller::format_for_noun_groups,"ПРИЛ_СУЩ", FROM_LEFT}},
		{rule_for_number_adverb, {(CFormatFunc)&CRusFormatCaller::format_for_number_adverb,"НАР_ЧИСЛ_СУЩ", FROM_LEFT}},
		{rule_for_number_noun, {(CFormatFunc)&CRusFormatCaller::format_for_number_noun,"ЧИСЛ_СУЩ", FROM_LEFT}},
		{rule_for_rank_surname, {(CFormatFunc)&CRusFormatCaller::format_for_rank_surname,"ДОЛЖ_ФИО", FROM_LEFT}},
		{rule_for_gen_chains, {(CFormatFunc)&CRusFormatCaller::format_for_gen_chains,"ГЕНИТ_ИГ", FROM_LEFT}},
		{rule_for_from_compar, {(CFormatFunc)&CRusFormatCaller::format_for_from_compar,"ОТСРАВН",FROM_LEFT}},
		{rule_for_preps, {(CFormatFunc)&CRusFormatCaller::format_for_preps,"ПГ", FROM_LEFT}},
		{rule_for_prep_IZ, {(CFormatFunc)&CRusFormatCaller::format_for_prep_IZ,"ПГ (из)", FROM_LEFT}},
		{rule_for_selective_groups, {(CFormatFunc)&CRusFormatCaller::format_for_selective_groups,"ЭЛЕКТ_ИГ", FROM_LEFT}},
		{rule_for_similar_noun_groups, {(CFormatFunc)&CRusFormatCaller::format_for_similar_noun_groups,"ОДНОР_ИГ", FROM_LEFT}},
		{rule_for_neg_verb, {(CFormatFunc)&CRusFormatCaller::format_for_neg_verb, "ОТР_ФОРМА", FROM_LEFT}}, 
		{ rule_for_dir_obj, {(CFormatFunc)&CRusFormatCaller::format_for_dir_obj, "ПРЯМ_ДОП", FROM_LEFT}},
		{ rule_for_instr_obj, {(CFormatFunc)&CRusFormatCaller::format_for_instr_obj,"ИНСТР_ДОП", FROM_LEFT}},
		{ rule_for_dir_obj_rev, {(CFormatFunc)&CRusFormatCaller::format_for_dir_obj_rev,"ПРЯМ_ДОП_РЕВ", FROM_LEFT}},
		{ rule_for_verb_inf, {(CFormatFunc)&CRusFormatCaller::format_for_verb_inf,"ПЕР_ГЛАГ_ИНФ", FROM_LEFT}},
		{rule_for_adv_adv, {(CFormatFunc)&CRusFormatCaller::format_for_adv_adv,"НАР_НАР", FROM_LEFT}}, 
		{rule_adv_number_adverb, {(CFormatFunc)&CRusFormatCaller::format_for_adv_number_adverb,"НАР_НАР_ЧИСЛ", FROM_LEFT}},
		{rule_for_approx_prep_noun, {(CFormatFunc)&CRusFormatCaller::format_for_approx_noun_prep,"АППРОКС_ПГ", FROM_LEFT}},
		{rule_for_partic_clause, {(CFormatFunc)&CRusFormatCaller::format_for_partic_clause,"ПРИЧ_СУЩ",FROM_RIGHT}},
		{rule_for_whose_clause, {(CFormatFunc)&CRusFormatCaller::format_for_whose_clause,"ПРИДАТ_ОПР",FROM_RIGHT}},
		{rule_for_adj_in_commas_preposition, {(CFormatFunc)&CRusFormatCaller::format_for_adj_in_commas_preposition,"ПРИЛ-СУЩ",FROM_RIGHT}},
		{rule_for_plural_noun_adj, {(CFormatFunc)&CRusFormatCaller::format_for_plural_noun_adj,"ПРИЛ-СУЩ",FROM_LEFT}},
		{rule_for_noun_adj_postposition, {(CFormatFunc)&CRusFormatCaller::format_for_noun_adj_postposition,"ПРИЛ_ПОСТПОС",FROM_LEFT}},
		{rule_for_noun_detached_adj_postposition, {(CFormatFunc)&CRusFormatCaller::format_for_noun_detached_adj_postposition,"СУЩ_ОБС_ПРИЛ",FROM_LEFT}},
		{rule_for_keyb, {(CFormatFunc)&CRusFormatCaller::format_for_keyb,"КЛВ",FROM_LEFT}},
		{rule_for_disrupt_conj, {(CFormatFunc)&CRusFormatCaller::format_for_disrupt_conj,"РАЗРЫВ_СОЮЗ",FROM_LEFT}},
		{rule_for_adv_predik, {(CFormatFunc)&CRusFormatCaller::format_for_adv_predik,"НАР_ПРЕДИК", FROM_LEFT}},
		{rule_for_anat_compar, {(CFormatFunc)&CRusFormatCaller::format_for_anat_compar,"АНАТ_СРАВН", FROM_LEFT}},
		{rule_for_foreign_term, {(CFormatFunc)&CRusFormatCaller::format_for_foreign_term,"ИНОСТР_ЛЕКС",FROM_LEFT}},
		{rule_for_preps_adj, {(CFormatFunc)&CRusFormatCaller::format_for_preps_adj,"ПГ (для прилагательных)", FROM_LEFT}},
		{rule_for_oborots, {(CFormatFunc)&CRusFormatCaller::format_for_oborots, "ОБОРОТ", FROM_LEFT}}
	};
};


 
int   CRusFormatCaller::GetRuleByGroupTypeForThesaurus(int GroupType) const
{
	switch (GroupType) 
	{
		case NOUN_ADJ:		return rule_for_noun_groups;
		case PREP_NOUN: 	return rule_for_preps;
		default: return -1;
	};
};



	/*
		Функция InitForSimilarAdj инициализирует порядок правил для  однородных  прилагательных.
		Форматка rule_for_similar_adjs применяется дважды. Сначала  строятся  группы однородных прилагательных,
		которые не включают в качестве самостоятельных членов  слова "самый" и "такой" .
		Например, для фразы 
			"стройный, такой красивый и молодой"
		будет построена группа "красивый и молодой",а "стройный, такой" построена не будет.
		Далее прогоняется правило МОДИФ_ПРИЛ, которая строит для вышеуказанного примера группу 
			"такой красивый и молодой"
		Потом еще раз прогоняется правило ОДНОР_ПРИЛ, которое достраивает (строит)  уже все ОДНОР_ПРИЛ, поскольку
		слова "самый" и "такой" уже вошли в группы и стоят на зависимых  местах. Получается такое:
			(стройный и (такой (красивый и молодой)))
		Интуитивно этот порядок применения правил говорит о том, что "такой" и "самый" в однородном ряде всегда распространяется на все 
		идущие после него члены. Однако по этому же правилу поучается, что
				"большой, стройный и такой красивый и молодой"
			строится так 			( (большой, стройный) и (такой (красивый и молодой)) ),

			А фраза "стройный, такой красивый и совсем молодой"
			строится как  (стройный, (такой (красивый и (совсем молодой))) ).

		Это все ошибки, которые надо исправить.	
	*/

static void InitForSimilarAdj( CFormatCaller& FormatCaller)
{
	FormatCaller.m_vectorRulesNumbers.push_back(rule_for_similar_adjs);
	FormatCaller.m_vectorRulesNumbers.push_back(rule_for_modif_adj);
	FormatCaller.m_vectorRulesNumbers.push_back(rule_for_similar_adjs);
};

void CRusFormatCaller::AddAllRules()
{
	// первыми всегда  должны   стоять правила, которые основываются только на графематике
	m_vectorRulesNumbers.push_back(rule_for_names);
	m_vectorRulesNumbers.push_back(rule_for_web_addr);
	m_vectorRulesNumbers.push_back(rule_for_keyb);
	m_vectorRulesNumbers.push_back(rule_for_oborots);
	

	m_vectorRulesNumbers.push_back(rule_for_foreign_term);
	m_vectorRulesNumbers.push_back(rule_for_num_complex);
	m_vectorRulesNumbers.push_back(rule_for_numbers);
	m_vectorRulesNumbers.push_back(rule_for_similar_numerals);
	m_vectorRulesNumbers.push_back(rule_for_noun_num);
	
	m_vectorRulesNumbers.push_back(rule_for_anat_compar);
	m_vectorRulesNumbers.push_back(rule_for_adv_adj);
	m_vectorRulesNumbers.push_back(rule_adv_number_adverb);
	m_vectorRulesNumbers.push_back(rule_for_adv_adv);
	m_vectorRulesNumbers.push_back(rule_for_adv_predik);	

	InitForSimilarAdj(*this);

	m_vectorRulesNumbers.push_back(rule_for_similar_advs);
	m_vectorRulesNumbers.push_back(rule_for_similar_infs);
	m_vectorRulesNumbers.push_back(rule_for_similar_comp_adjs);
	m_vectorRulesNumbers.push_back(rule_for_comp_adv);
	m_vectorRulesNumbers.push_back(rule_for_adv_verb);

	m_vectorRulesNumbers.push_back(rule_for_number_adverb);/**/
	m_vectorRulesNumbers.push_back(rule_for_number_noun);
	m_vectorRulesNumbers.push_back(rule_for_approx_prep_noun);	


	m_vectorRulesNumbers.push_back(rule_for_noun_groups);
	m_vectorRulesNumbers.push_back(rule_for_plural_noun_adj);
	m_vectorRulesNumbers.push_back(rule_for_noun_adj_postposition);
	m_vectorRulesNumbers.push_back(rule_for_noun_detached_adj_postposition);
	m_vectorRulesNumbers.push_back(rule_for_number_noun); // "совершила более 40 тяжких преступлений"
	m_vectorRulesNumbers.push_back(rule_for_number_adverb);/**/
	m_vectorRulesNumbers.push_back(rule_for_number_noun);
	m_vectorRulesNumbers.push_back(rule_for_rank_surname);
	m_vectorRulesNumbers.push_back(rule_for_gen_chains);
	m_vectorRulesNumbers.push_back(rule_for_from_compar);
	m_vectorRulesNumbers.push_back(rule_for_prep_IZ);
	m_vectorRulesNumbers.push_back(rule_for_selective_groups);
	m_vectorRulesNumbers.push_back(rule_for_preps);
	m_vectorRulesNumbers.push_back(rule_for_similar_noun_groups);
	m_vectorRulesNumbers.push_back(rule_for_similar_noun_groups); //дублирование для примеров типа "стол и стул и окно"
	m_vectorRulesNumbers.push_back(rule_for_plural_noun_adj); // чтобы собрать "черные шкаф и стул"
	m_vectorRulesNumbers.push_back(rule_for_neg_verb);
	m_vectorRulesNumbers.push_back(rule_for_dir_obj);
	m_vectorRulesNumbers.push_back(rule_for_instr_obj);
	
	m_vectorRulesNumbers.push_back(rule_for_dir_obj_rev);
	m_vectorRulesNumbers.push_back(rule_for_similar_infs);
	m_vectorRulesNumbers.push_back(rule_for_verb_inf);
	m_vectorRulesNumbers.push_back(rule_for_disrupt_conj); 
	m_vectorRulesNumbers.push_back(rule_for_preps_adj); 

	m_vectorRulesNumbers.push_back(rule_for_noun_adjuncts);

	//  еще раз вызываем правило rule_for_gen_chains для фразы "руководителям отдела и всего проекта"
	m_vectorRulesNumbers.push_back(rule_for_gen_chains);

	//  еще раз вызываем правило rule_for_verb_inf для фразы "Он хотел научиться писать"
	m_vectorRulesNumbers.push_back(rule_for_verb_inf);

	//  для предложения   "Он изобрел и сад, и огород."
	m_vectorRulesNumbers.push_back(rule_for_dir_obj);
}

void CRusFormatCaller::AddRulesBeforeSimClauses( )
{
	// первыми всегда  должны   стоять правила, которые основываются только на графематике
	m_vectorRulesNumbers.push_back(rule_for_names);
	m_vectorRulesNumbers.push_back(rule_for_web_addr);
	m_vectorRulesNumbers.push_back(rule_for_keyb);	
	m_vectorRulesNumbers.push_back(rule_for_oborots);
	m_vectorRulesNumbers.push_back(rule_for_foreign_term);

	
	m_vectorRulesNumbers.push_back(rule_for_num_complex);
	m_vectorRulesNumbers.push_back(rule_for_numbers);
	m_vectorRulesNumbers.push_back(rule_for_similar_numerals);	
	m_vectorRulesNumbers.push_back(rule_for_noun_num);
	
	m_vectorRulesNumbers.push_back(rule_for_anat_compar);
	m_vectorRulesNumbers.push_back(rule_for_adv_adj);
	m_vectorRulesNumbers.push_back(rule_adv_number_adverb);
	m_vectorRulesNumbers.push_back(rule_for_adv_adv);
	m_vectorRulesNumbers.push_back(rule_for_adv_predik);	

	InitForSimilarAdj(*this);

	m_vectorRulesNumbers.push_back(rule_for_similar_advs);
	m_vectorRulesNumbers.push_back(rule_for_similar_infs);
	m_vectorRulesNumbers.push_back(rule_for_similar_comp_adjs);
	m_vectorRulesNumbers.push_back(rule_for_comp_adv);
	m_vectorRulesNumbers.push_back(rule_for_adv_verb);

	m_vectorRulesNumbers.push_back(rule_for_number_adverb);/**/
	m_vectorRulesNumbers.push_back(rule_for_number_noun);
	m_vectorRulesNumbers.push_back(rule_for_approx_prep_noun);	

	m_vectorRulesNumbers.push_back(rule_for_noun_groups);
	m_vectorRulesNumbers.push_back(rule_for_plural_noun_adj);
	m_vectorRulesNumbers.push_back(rule_for_noun_adj_postposition);
	m_vectorRulesNumbers.push_back(rule_for_noun_detached_adj_postposition);
	m_vectorRulesNumbers.push_back(rule_for_number_noun); // "совершила более 40 тяжких преступлений"
	m_vectorRulesNumbers.push_back(rule_for_number_adverb);/**/
	m_vectorRulesNumbers.push_back(rule_for_number_noun);
	m_vectorRulesNumbers.push_back(rule_for_rank_surname);
	m_vectorRulesNumbers.push_back(rule_for_gen_chains);	
	m_vectorRulesNumbers.push_back(rule_for_from_compar);	
	m_vectorRulesNumbers.push_back(rule_for_prep_IZ);
	m_vectorRulesNumbers.push_back(rule_for_selective_groups);	
	m_vectorRulesNumbers.push_back(rule_for_preps);
}

void CRusFormatCaller::AddRulesAfterSimClauses( )
{
	m_vectorRulesNumbers.push_back(rule_for_similar_noun_groups);
	m_vectorRulesNumbers.push_back(rule_for_similar_noun_groups); //дублирование для примеров типа "стол и стул и окно"
	m_vectorRulesNumbers.push_back(rule_for_neg_verb);
	m_vectorRulesNumbers.push_back(rule_for_dir_obj);
	m_vectorRulesNumbers.push_back(rule_for_instr_obj);
	
	m_vectorRulesNumbers.push_back(rule_for_dir_obj_rev);
	m_vectorRulesNumbers.push_back(rule_for_verb_inf);
	m_vectorRulesNumbers.push_back(rule_for_disrupt_conj); //&&&
}

void CRusFormatCaller::AddSimpleSimilarRules( )
{
	m_vectorRulesNumbers.push_back(rule_for_oborots);
	m_vectorRulesNumbers.push_back(rule_for_similar_numerals);
	m_vectorRulesNumbers.push_back(rule_for_adv_adj);
	m_vectorRulesNumbers.push_back(rule_for_adv_adv);
	m_vectorRulesNumbers.push_back(rule_for_adv_predik);
	m_vectorRulesNumbers.push_back(rule_for_adv_adj);

	InitForSimilarAdj(*this);

	m_vectorRulesNumbers.push_back(rule_for_similar_advs_comp_conj);
	m_vectorRulesNumbers.push_back(rule_for_similar_infs);
	m_vectorRulesNumbers.push_back(rule_for_similar_comp_adjs);
	m_vectorRulesNumbers.push_back(rule_for_noun_groups);
}

void CRusFormatCaller::AddGroupRulesForClause( )
{
	m_vectorRulesNumbers.push_back(rule_for_partic_clause);
	m_vectorRulesNumbers.push_back(rule_for_whose_clause);
	m_vectorRulesNumbers.push_back(rule_for_adj_in_commas_preposition);
	m_vectorRulesNumbers.push_back(rule_for_plural_noun_adj);
	m_vectorRulesNumbers.push_back(rule_for_noun_adj_postposition);
	m_vectorRulesNumbers.push_back(rule_for_noun_detached_adj_postposition); //???
	m_vectorRulesNumbers.push_back(rule_for_disrupt_conj); //&&&
}

void CRusFormatCaller::AddSimilarNounGroupRules(  )
{	
	m_vectorRulesNumbers.push_back(rule_for_oborots);
	m_vectorRulesNumbers.push_back(rule_for_noun_num);
    m_vectorRulesNumbers.push_back(rule_for_number_noun);
	m_vectorRulesNumbers.push_back(rule_for_noun_groups);
	m_vectorRulesNumbers.push_back(rule_for_plural_noun_adj);
	m_vectorRulesNumbers.push_back(rule_for_noun_adj_postposition);
	m_vectorRulesNumbers.push_back(rule_for_noun_detached_adj_postposition);
	m_vectorRulesNumbers.push_back(rule_for_similar_noun_groups);
	m_vectorRulesNumbers.push_back(rule_for_similar_advs);
}

void CRusFormatCaller::AddSimpleRulesForAnalyticalVerbFormChecking( )
{
	m_vectorRulesNumbers.push_back(rule_for_oborots);
	m_vectorRulesNumbers.push_back(rule_for_noun_num);
    m_vectorRulesNumbers.push_back(rule_for_number_noun);
	m_vectorRulesNumbers.push_back(rule_for_noun_groups);
	m_vectorRulesNumbers.push_back(rule_for_plural_noun_adj);
	m_vectorRulesNumbers.push_back(rule_for_prep_IZ);
	m_vectorRulesNumbers.push_back(rule_for_selective_groups);	
	m_vectorRulesNumbers.push_back(rule_for_preps);
}



bool CRusFormatCaller::IsSimilarConnector(const CSynPlmLine& L) const
{
    if (L.HasFlag(fl_comma) ) return true;
    if (!L.HasPOS(CONJ)) return false;
    int conj_no = L.get_coord_conj_no();
	return		(conj_no!= -1)  &&	!GetOpt()->GetOborDic()->GetCoordConjs()[conj_no].should_be_after_comma;
		

};
void CRusFormatCaller::BuildOborotGroups() 
{
    CIntVector save = m_vectorRulesNumbers;
    m_vectorRulesNumbers.clear();
    m_vectorRulesNumbers.push_back(rule_for_oborots);
    main_analyse();
    swap (m_vectorRulesNumbers,  save);

}


