#include "MAPostMain.h"
#include "morph_dict/lemmatizer_base_lib/rus_numerals.h"
#include "morph_dict/agramtab/agramtab.h"


bool CMAPost::is_russian_numeral(std::string& word) const {
	std::vector<CFormInfo> Paradigms;
    auto s8 = _R(word);
	m_pRusLemmatizer->CreateParadigmCollection(false, s8, false, false, Paradigms);
	for (auto& p : Paradigms)
	{
		auto form = convert_to_utf8(p.GetWordForm(0), morphRussian);
		if (RussianNumerals.CheckIsNumeral(form)) {
			return true;
		}
	}
	return false;
}

std::string CMAPost::GetSimilarNumAncode(const std::string& Lemma, const std::string& Flexia, bool IsNoun)
{
	if (Lemma.length() == 0) return "";
	std::vector<CFormInfo> Paradigms;
	std::string h = _R(Lemma);
	m_pRusLemmatizer->CreateParadigmCollection(false, h, false, false, Paradigms);
	if (Paradigms.size() == 0) return ""; // например, нету в Ё-словаре слова ЧЕТВЕРТЫЙ
	// ищем числительное
	long k = 0;
	for (; k < Paradigms.size(); k++)
	{
		std::string AnCode = Paradigms[k].GetAncode(0);
		BYTE POS = m_pRusGramTab->GetPartOfSpeech(AnCode.c_str());
		if (IsNoun)
		{
			if (POS == NOUN)
				break;
		}
		if ((POS == NUMERAL) || (POS == NUMERAL_P) || Lemma == "НУЛЕВОЙ")
			break;
	};
	assert(k < Paradigms.size());
	const CFormInfo& P = Paradigms[k];

	// ищем максимальное совпадение с конца 
	std::string AnCodes;
	for (k = 0; k < P.GetCount(); k++)
	{
		std::string Form = convert_to_utf8(P.GetWordForm(k), morphRussian);
		MakeLowerUtf8(Form);
		if (IsNoun && Form != h && m_AbbrIndeclGramCodes.find(P.GetAncode(k)) != m_AbbrIndeclGramCodes.end())
            // 1000 - не аббр, "свыше 1000 человек"
			continue;

		if (Form.length() > Flexia.length())
			if (Flexia == "" || endswith(Form, Flexia))
				AnCodes += P.GetAncode(k);
	};

	return m_pRusGramTab->UniqueGramCodes(AnCodes);


};


void CMAPost::Cifrdef()
{

	// Ищем последовательность чисел и окончаний типа
	// 1960-го                           2 6 DSC
	//и лемматизируем этого слово как ЧИСЛ и ЧИСЛ_П
	CLineIter dollar = m_Words.end();
	for (CLineIter it = m_Words.begin(); it != m_Words.end(); it++)
	{
		CLemWord& W = *it;
		CLineIter next_it = it;
		CLineIter prev_it = it;
		next_it++;
		if (it != m_Words.begin()) prev_it--;

		size_t hyp = W.m_strWord.find("-");

		bool foundHyp = (hyp != std::string::npos) && (hyp > 0);
		if (W.IsInOborot()) continue;

		// Доллары
		if (!isdigit((BYTE)W.m_strWord[0]) && !(foundHyp && CheckRussianUtf8(W.m_strWord.substr(hyp))))
			if (dollar == prev_it)//$9,4 млрд
			{
				if (is_russian_numeral(W.m_strWord)) {
					if ((*it).HasDes(OSentEnd)) { 
						(*it).DelDes(OSentEnd); 
						(*prev_it).AddDes(OSentEnd); 
					}
					iter_swap(prev_it, it);
					dollar++;
				}
			}
			else
				if (isdigit((BYTE)W.m_strWord.back()) && foundHyp && CheckRussianUtf8(W.m_strWord.substr(0, hyp)))
				{
					W.DelDes(ONumChar);
					W.AddDes(ORLE);
					W.DeleteAllHomonyms();
					CHomonym* pNew = W.AddNewHomonym();
					std::vector<CFormInfo> paradigms;
					std::string s8 = _R(W.m_strWord.substr(0, hyp));
					m_pRusLemmatizer->CreateParadigmCollection(false, s8, false, false, paradigms);
					pNew->SetLemma(W.m_strUpperWord);
					if (!paradigms.empty()) // плутония-238
					{
						pNew->CopyFromFormInfo(&paradigms[0]);
						W.m_bWord = true;
					}
					else
					{
						pNew->SetPredictedWord(m_DURNOVOGramCode);
					}
				}
				else
					continue;
		// первая часть - цифры, второая - русская, если есть окончание
		std::string NumWordForm = !foundHyp ? it->m_strWord : it->m_strWord.substr(0, hyp);
		std::string Flexia = !foundHyp ? "" : it->m_strWord.substr(hyp + 1);

		if (Flexia == "" && next_it != m_Words.end() && isdigit((BYTE)next_it->m_strWord[0]) && next_it->m_strWord.length() == 3) // "в 1 300 световых годах" -> 1300
		{
			next_it->m_strWord = W.m_strWord + next_it->m_strWord;
			W.DeleteAllHomonyms();
			m_Words.erase(it, it);
			continue;
		}
		//  Идем с  конца ищем числительное, которое максимально совпадает с конца с числительным во входном тексте.
		std::string NumWordForm2 = NumWordForm;
		while (atoi(NumWordForm2.c_str()) >= 1000 && NumWordForm2.substr(NumWordForm2.length() - 3) == "000")
			NumWordForm2 = NumWordForm2.substr(0, NumWordForm2.length() - 3);

		const CNumeralToNumber* numeral = nullptr;
		for (auto& n: RussianNumerals.GetAllNumeralReverse())
		{
			if (n.m_Number == 0 && NumWordForm != "0") {
				continue;
			}
			std::string NumValue;
			if (W.HasDes(ORoman))
				NumValue = n.m_RomanNumber;
			else
				NumValue = DoubleToStr(n.m_Number);

			if (NumValue.length() > 0 && endswith(NumWordForm2, NumValue))
			{
				numeral = &n;
				break;
			}
		};
		if (numeral == nullptr)  continue;

		MakeLowerUtf8(Flexia);
		std::string AnCodes = GetSimilarNumAncode(numeral->m_Cardinal, Flexia, numeral->m_bNoun);
		if (AnCodes.empty() && Flexia != "")
			AnCodes = GetSimilarNumAncode(numeral->m_Ordinal, Flexia, numeral->m_bNoun);
		if (numeral->m_Cardinal == "ОДИН") {
			AnCodes =  m_pRusGramTab->GramCodes().m_GenderNumeral;
		}
		std::string AnCodes0 = AnCodes; //числ
		if (NumWordForm != "0") {
			AnCodes = GetSimilarNumAncode(numeral->m_Ordinal, Flexia, numeral->m_bNoun);
		}
		if (Flexia == "") {
			// 1. удаляем грамкоды множес. числа
			// можно просто не удалять, тогда будет очень много вариантов на синтаксисе (чистая оптимизация)
			//2. добавил условие для примера  "в 1960 годы", где порядковое существительное 1960 стоит в отчетливом множественном числе
			if (W.m_strWord.length() != 4) { // "1960"
				AnCodes = m_pRusGramTab->FilterGramCodes1(AnCodes, _QM(rSingular), 0);
			}
		}
		if (FindFloatingPoint(NumWordForm) != -1 || AnCodes0 == AnCodes)
			AnCodes = "";
		if (AnCodes.empty() && AnCodes0.empty())
		{
			// "20-летний"
			if (W.LemmatizeFormUtf8(Flexia, m_pRusLemmatizer))
			{
				W.DelDes(ONumChar);
				W.AddDes(ORLE);
				W.SetWordStr(NumWordForm + "#" + Flexia);
			}
		}
		else
		{

			W.AddDes(ORLE);
			W.AddDes(OLw);
			W.DeleteAllHomonyms();

			if (!(AnCodes0.empty() || (next_it != m_Words.end() && (next_it->m_strUpperWord == "ГГ")))) // ЧИСЛ
			{
				CHomonym* pH = W.AddNewHomonym();
				pH->SetPredictedWord(AnCodes0);
				pH->SetLemma(NumWordForm);
			}
			CLineIter spec_it = it;
			if (next_it != m_Words.end() && (next_it->m_strWord == "%" || next_it->m_strWord == "$"))  //доллары, проценты
				spec_it = next_it;
			else if (it != m_Words.begin() && (prev_it->m_strWord == "$" || prev_it->m_strWord == "№"))
				spec_it = prev_it;
			if (it != spec_it)
			{
				CLemWord& W2 = *spec_it;
                LOGV << "apply Cifrdef to  " <<  W2.m_strWord;
				std::vector<CFormInfo> Paradigms;
				W2.DeleteOborotMarks();
				W2.AddDes(ORLE);
				W2.DelDes(OPun);
				W2.DeleteAllHomonyms();
				CHomonym* pH = W2.AddNewHomonym();
				pH->SetPredictedWord(m_pRusGramTab->GramCodes().m_MasAbbrNoun,
					m_pRusGramTab->GramCodes().m_InanimIndeclNoun);
				if (W2.m_strWord == "%")
				{
					W2.m_strUpperWord = W2.m_strWord = "ПРОЦ";
					pH->SetLemma("ПРОЦЕНТ");
				}
				else  if (W2.m_strWord == "$")
				{
					if (spec_it == prev_it) //$12
						dollar = prev_it;
					W2.m_strUpperWord = W2.m_strWord = "ДОЛЛ";
					pH->SetLemma("ДОЛЛАР");
				}
				else  if (W2.m_strWord == "№")
				{
					W2.m_strUpperWord = W2.m_strWord = "№";
					pH->SetLemma("НОМЕР");
				}
                auto s8 = _R(pH->GetLemma());
				m_pRusLemmatizer->CreateParadigmCollection(true, s8, true, false, Paradigms);
				pH->m_lPradigmID = Paradigms[0].GetParadigmId();
				pH->m_SearchStatus = DictionaryWord;
				pH->InitAncodePattern();

				W2.m_bWord = true;
			}
			else
				if (!((next_it != m_Words.end() && (next_it->m_strUpperWord == "ЛЕТ"))	// "в течение 2 лет"
					))
					if (!AnCodes.empty())  //ЧИСЛ-П
					{
						CHomonym* pH = W.AddNewHomonym();
						pH->SetPredictedWord(AnCodes);
						pH->SetLemma(NumWordForm);
					}
			if (dollar == prev_it)
			{
				if ((*it).HasDes(OSentEnd)) { (*it).DelDes(OSentEnd); (*prev_it).AddDes(OSentEnd); }
				iter_swap(prev_it, it);
				dollar++;
			}
		}

	};

};

