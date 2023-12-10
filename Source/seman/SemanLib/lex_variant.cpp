#include "stdafx.h"
#include "SemanticRusStructure.h"

struct CProductivePrefix {
    std::string m_Prefix;
    std::string m_GraDescr;
};

const std::vector<CProductivePrefix> Prefixes = {{"АВИА", "АВИА"},
                                                 {"АЭРО", "АВИА"}};


part_of_speech_mask_t CSemanticStructure::GetPosesFromRusArticle(CStructDictHolder &Ross, uint16_t UnitNo) const {
    StringVector GramFets;
    Ross.GetSimpleFieldItemsFromArticle(UnitNo, "GF", 0, 0, GramFets);
    if (GramFets.size() == 0) return 0;
    long ret_poses = 0;
    for (long i = 0; i < GramFets.size(); i++) {
        std::string S = GramFets[i].substr(0, GramFets[i].find_first_of(":"));

        if (S == "СУЩ")
            ret_poses |= (1 << NOUN);
        else if (S == "МС")
            ret_poses |= (1 << PRONOUN);
        else if ((S == "МЕСТОИМ") || (S == "МС-П") || (S == "ПРИТ_МЕСТМ"))
            ret_poses |= (1 << PRONOUN_P);
        else if (S == "НАР")
            ret_poses |= (1 << ADV);
        else if (S == "ЧАСТ")
            ret_poses |= (1 << PARTICLE);
        else if (S == "ПРЕДК")
            ret_poses |= (1 << PREDK) | (1 << PRONOUN_PREDK);
        else if (S == "ГЛ")
            ret_poses |= ((1 << VERB) |
                          (1 << ADVERB_PARTICIPLE) |
                          (1 << PARTICIPLE) |
                          (1 << PARTICIPLE_SHORT) |
                          (1 << INFINITIVE));
        else if (S == "ПРИЛ")
            ret_poses |= ((1 << ADJ_FULL)
                          | (1 << ADJ_SHORT)
                          | (1 << PARTICIPLE)
                          | (1 << PARTICIPLE_SHORT)
            );
        else if (S == "ЧИСЛ")
            ret_poses |= ((1 << NUMERAL_P) | (1 << NUMERAL));
        else {
            ErrorMessage(std::string("Unknown POS ") + std::string(GramFets[i])
                         + std::string(" in article ")
                         + std::string(Ross.GetRoss()->GetEntryStr(UnitNo)));

            return 0;
        };
    }
    return ret_poses;
}

bool CRusSemStructure::VerbCanBeWithoutReflexiveSuffix(std::string lemma) const {
    if (!HasReflexiveSuffix(lemma)) return false;
    lemma.erase(lemma.length() - ReflexiveSuffixLen);
    std::vector<CFormInfo> ParadigmCollection;
    auto s8 = convert_from_utf8(lemma.c_str(), morphRussian);
    m_pData->GetRusLemmatizer()->CreateParadigmCollection(true, s8, false, false, ParadigmCollection);

    for (auto& p: ParadigmCollection) {
        part_of_speech_t POS = m_pData->GetRusGramTab()->GetPartOfSpeech(p.GetAncode(0).c_str());
        if (POS == INFINITIVE) return true;
    };
    return false;

};

std::string CRusSemStructure::GetNormOfParticiple(const CRusSemWord &W) const {
    uint64_t Grammems = _QM(rMasculinum) | _QM(rSingular) | _QM(rNominativ);
    if (W.HasOneGrammem(rActiveVoice))
        Grammems |= _QM(rActiveVoice);
    if (W.HasOneGrammem(rPassiveVoice))
        Grammems |= _QM(rPassiveVoice);
    // строим по краткой форме_R( "обеспокоен") форму_R( "обеспокоенный")
    if (W.HasOneGrammem(rPastTense) || W.HasPOS(PARTICIPLE_SHORT))
        Grammems |= _QM(rPastTense);
    else if (W.HasOneGrammem(rPresentTense))
        Grammems |= _QM(rPresentTense);
    if (W.m_ParadigmId == UnknownParadigmId) return "error_!";
    CFormInfo Paradigm;
    if (!m_pData->GetRusLemmatizer()->CreateParadigmFromID(W.m_ParadigmId, Paradigm))
        return "error_!";


    long k = 0;
    for (; k < Paradigm.GetCount(); k++) {
        std::string AnCode = Paradigm.GetAncode(k);
        uint64_t currGrammems;
        m_pData->GetRusGramTab()->GetGrammems(AnCode.c_str(), currGrammems);
        if ((Grammems & currGrammems) == Grammems)
            break;
    };
    if (k == Paradigm.GetCount()) return "error_!";

    return convert_to_utf8(Paradigm.GetWordForm(k), morphRussian);
};


uint16_t CRusSemStructure::GetInterpWithoutPrefix(CRusSemWord &W) const {
    for (auto& p : Prefixes)
        if (startswith(W.m_Lemma, p.m_Prefix)) {
            std::string UnitStr = p.m_Prefix;
            MakeLowerUtf8(UnitStr);
            uint16_t UnitNo = GetRossHolder(Ross)->LocateUnit(UnitStr.c_str(), 1);
            if (UnitNo == ErrUnitNo) continue;

            uint32_t ParadigmId = m_pData->GetFirstParadigmId(morphRussian, UnitStr, W.m_Poses);
            if (ParadigmId == UnknownParadigmId) continue;
            W.m_Lemma = UnitStr;
            W.m_GraphDescrs += " " + p.m_GraDescr;
            W.m_ParadigmId = ParadigmId;
            W.SetWord(W.GetWord().substr(p.m_Prefix.length()));
            return UnitNo;
        };
    return ErrUnitNo;
};


void CRusSemStructure::InitInterps(CRusSemWord &W, bool PassiveForm, long ClauseNo, CRusSemNode &N,
                                   bool UsePlugArticle) const {
    try {
        N.DelAllInterps();

        if (W.m_IsPunct) return;

        //==================================================================
        // ========	 Интерпретация по РОССу	(основной словарь)
        //=================================================================

        // ищем	словарную статью в РОССе по	лемме
        // Например, обеспокоенный
        std::string UnitStr;
        uint16_t UnitNo = ErrUnitNo;
        if (   //( W.m_Poses & (1	<< PARTICIPLE) )
                (W.m_Poses & (1 << PARTICIPLE_SHORT))
                ) {
            UnitStr = GetNormOfParticiple(W);
            MakeLowerUtf8(UnitStr);
            UnitNo = GetRossHolder(Ross)->LocateUnit(UnitStr.c_str(), 1);
            /*
            (1)	Там	были лица, не заинтересованные в нормализации обстановки
            (2)	они	обеспокоены	принятием нового трудового кодекса
            В втором  примере нужно	убрать	пассивность, поскольку здесь
            "обеспокоены" выступает	в роли краткого	прилагательного. Иначе меняются	местами
            валентности.
            */
            if (UnitNo != ErrUnitNo)
                PassiveForm = false;

        }


        if (UnitNo == ErrUnitNo) {
            UnitStr = W.m_Lemma;
            MakeLowerUtf8(UnitStr);
        };

        bool bHasSuffixFemine = true;

        TryWithoutFeminSuffix:

        if (UnitNo == ErrUnitNo) {
            UnitNo = GetRossHolder(Ross)->LocateUnit(UnitStr.c_str(), 1);
        };

        // если	слово не нашлось, то постараемся найти его видовую пару
        if (UnitNo == ErrUnitNo)
            for (auto& s: W.m_WordEquals) {
                UnitStr = s;
                MakeLowerUtf8(UnitStr);
                UnitNo = GetRossHolder(Ross)->LocateUnit(UnitStr.c_str(), 1);
                if (UnitNo != ErrUnitNo) break;
            };

        /*
        пробуем	отделить  от слова приставки типа (авиа-)
        */
        if (UnitNo == ErrUnitNo)
            UnitNo = GetInterpWithoutPrefix(W);


        // поиск поля REF
        bool ReferenceFound = false;
        if (UnitNo != ErrUnitNo) {
            if (!GetRoss(Ross)->IsEmptyArticle(UnitNo))
                for (size_t i = GetRoss(Ross)->GetUnitStartPos(UnitNo);
                     i <= GetRoss(Ross)->GetUnitLastPos(UnitNo); i++) {
                    if (GetRoss(Ross)->GetCortegeFieldNo(i) == GetRossHolder(Ross)->RefFieldNo) {
                        TCortege C = GetRossHolder(Ross)->GetCortegeCopy(i);
                        std::string S = GetRossHolder(Ross)->GetDomItemStrWrapper(C.GetItem(0));
                        long MeanNum = -1;
                        if (!C.is_null(1)) {
                            std::string Q = GetRossHolder(Ross)->GetDomItemStrWrapper(C.GetItem(1));
                            if (Q.length() == 1) {
                                MeanNum = Q[0] - '0';
                            };
                        };
                        if (MeanNum == -1)
                            ErrorMessage("Reference without MeanNum used");
                        else {
                            ReferenceFound = true;
                            long UnitNo1 = GetRossHolder(Ross)->LocateUnit(S.c_str(), MeanNum);
                            if (UnitNo1 != ErrUnitNo)
                                if (GramFetAgreeWithPoses(*GetRossHolder(Ross), UnitNo1, W))
                                    N.AddInterp(
                                            CDictUnitInterp(GetRossHolder(Ross), Ross, UnitNo1, false, PassiveForm));
                        }
                    }
                };
        };

        if (UnitNo != ErrUnitNo)
            if (!ReferenceFound) {
                for (long i = UnitNo; i < GetRoss(Ross)->GetUnitsSize() && GetRoss(Ross)->GetEntryStr(UnitNo) == GetRoss(Ross)->GetEntryStr(i); i++) {
                    if (GramFetAgreeWithPoses(*GetRossHolder(Ross), i, W))
                        N.AddInterp(CDictUnitInterp(GetRossHolder(Ross), Ross, i, false, PassiveForm));
                }
            }

        if (N.GetInterps().empty()
            && HasReflexiveSuffix(W.m_Lemma)
            && VerbCanBeWithoutReflexiveSuffix(W.m_Lemma)
            // все глаголы на СЯ не	являются транзитивными
            && W.HasOneGrammem(rNonTransitive)
                ) {
            // сохраняем суффикс -СЯ, чтобы	можно было его добавить
            std::string suffix = W.m_Lemma.substr(W.m_Lemma.length() - ReflexiveSuffixLen);
            W.m_Lemma = W.m_Lemma.substr(0, W.m_Lemma.length() - ReflexiveSuffixLen);
            // подключаем транзитивность вместо	нетранзитивности
            W.SetTypeGrammems((W.GetTypeGrammems() & ~_QM(rNonTransitive)) | _QM(rTransitive));
            StringVector SuffixAspVerb;
            SuffixAspVerb.resize(W.m_WordEquals.size());
            size_t j = 0;
            for (auto& e: W.m_WordEquals) {
                // не всегда глагол	на -ся имеет аспектную пару	с -ся.
                // например	"садиться"-"сесть"
                if (HasReflexiveSuffix(e)) {
                    SuffixAspVerb[j] = e.substr(e.length() - ReflexiveSuffixLen);
                    e = e.substr(0, e.length() - ReflexiveSuffixLen);
                } else {
                    SuffixAspVerb[j] = e;
                    e = "";
                };
                ++j;
            };


            InitInterps(W, true, ClauseNo, N);
            // ... обратно
            W.SetTypeGrammems((W.GetTypeGrammems() & ~_QM(rTransitive)) | _QM(rNonTransitive));

            // добавляем суффикс СЯ	обратно, чтобы могло собраться словосочетание
            // "остановиться в развитии"

            W.m_Lemma += suffix;
            for (long j = 0; j < W.m_WordEquals.size(); j++)
                W.m_WordEquals[j] += SuffixAspVerb[j];

            return;
        };

        //==================================================================
        // ========	 Интерпретация по другим словарям
        //=================================================================

        GetTimeInterps(UnitStr, W.GetWord(), W.m_PostPuncts, N);

        GetThesInterps(UnitStr, W, m_pData->GetThes(OmniThes), OmniRoss, N);
        GetThesInterps(UnitStr, W, m_pData->GetThes(FinThes), FinRoss, N);
        GetThesInterps(UnitStr, W, m_pData->GetThes(CompThes), CompRoss, N);
        GetThesInterps(UnitStr, W, m_pData->GetThes(LocThes), LocRoss, N);

        /*запустим эту функцию перед статьями заглушками*/
        DelProhibitedInterps(UnitStr, N);

        if (N.GetInterps().empty())
            if (bHasSuffixFemine
                && W.HasPOS(NOUN)
                && W.HasOneGrammem(rFeminum)
                && W.HasOneGrammem(rAnimative)
                    ) {
                bHasSuffixFemine = false;
                if (BuildWordWithoutFemineSuffix(UnitStr)) {
                    /*
                    для	"учительницы" кладем в массив эквивалентов слово "учитель"
                    */
                    W.m_WordEquals.push_back(UnitStr);

                    goto TryWithoutFeminSuffix;
                };
            };


        if (UsePlugArticle)
            if (N.GetInterps().empty()) {
                long ArtNo = FindAbstractPlugArticle(Ross, W.GetAllGrammems(), W.m_Poses,
                                                     m_Clauses[ClauseNo].m_ClauseType);
                if (ArtNo != -1)
                    N.AddInterp(CDictUnitInterp(GetRossHolder(Ross), Ross,
                                                (*m_pData->GetAbstractArticles(Ross))[ArtNo].m_UnitNo, false,
                                                PassiveForm));


            }
        DelProhibitedInterps(UnitStr, N);

    }
    catch (std::exception& e) {
        LOGE << e.what() << " for word " << W.GetWord();
        throw;
    }
    catch (...) {
        throw CExpc("Bad dict interpretation");
    };
};



CRusSemNode CRusSemStructure::CreatePrimitiveNode(size_t WordNo) {

    long ClauseNo = m_piSent->GetPrimitiveClauseNoByClauseNo(m_piSent->m_Words[WordNo].m_iClauseNo);
    CRusSemNode N;
    const CSynWord &W = m_piSent->m_Words[WordNo];

    CSynHomonym H(morphRussian);
    H.SetSentence(const_cast<CSentence *>(m_piSent));
    GetHomonym(ClauseNo, WordNo, H);

    unsigned short UnitNo = ErrUnitNo;
    CRusSemWord SemWord(WordNo, H.GetLemma());
    SemWord.m_SynWordNo = WordNo;
    SemWord.m_ParadigmId = H.m_lPradigmID;
    std::string TypeAncode;
    SemWord.SetFormGrammems(H.m_iGrammems);
    {
        //  reading grammems from "type ancode"

        if ((H.m_CommonGramCode.length() > 1) && (H.m_CommonGramCode != "??")) {
            TypeAncode = H.m_CommonGramCode;
            SemWord.SetTypeGrammems(H.m_CommonGramCode.length() > 2 ? H.m_TypeGrammems
                                                                    : m_pData->GetRusGramTab()->GetAllGrammems(
                            TypeAncode.c_str()));
        };

    }
    SemWord.m_Poses = H.m_iPoses;
    std::string GramCodes = H.GetGramCodes();
    SemWord.m_pData = m_pData;
    SemWord.m_GramCodes = GramCodes;


    SemWord.InitWordFeatures(m_piSent, WordNo);

    if (SemWord.HasPOS(PARTICIPLE_SHORT)
        && SemWord.HasOneGrammem(rPerfective)
        && SemWord.HasOneGrammem(rPastTense)
        && (SemWord.GetWord().find("-") == std::string::npos)
            ) {
        //  будем считать, что формы кратких причастий "св,стр,прш" настояшим временем
        // Например:
        // "арестован"
        // "разбросан"
        SemWord.SetFormGrammems((SemWord.GetFormGrammems() & ~_QM(rPastTense)) | _QM(rPresentTense));
    };

    if (!HasAuxiliaryPOS(SemWord.m_Lemma)) {
        if (SemWord.m_ParadigmId != -1) {
            CFormInfo Paradigm;
            bool bRes = m_pData->GetRusLemmatizer()->CreateParadigmFromID(SemWord.m_ParadigmId, Paradigm);
            assert (bRes);
            SemWord.m_WordWeight = Paradigm.GetWordWeight();
        };
    } else {
        // todo: delete it after sorting by word/hom weights
        if (SemWord.m_Lemma == "ОН" ) {
            SemWord.m_WordWeight = 100; //WordWeightCountCoef is -0.01
        }
        if (SemWord.m_Lemma == "ОНО" ) {
            SemWord.m_WordWeight = 5;
        }
    }

    if (SemWord.HasOneGrammem(rPerfective))
        SemWord.m_WordEquals = m_pData->GetAspVerb(SemWord.m_ParadigmId, true);

    if (SemWord.HasOneGrammem(rNonPerfective))
        SemWord.m_WordEquals = m_pData->GetAspVerb(SemWord.m_ParadigmId, false);



    // Добавляем операторы, которые пришли из морфологии (они были помещены в графеты
    // на Mapostе)

    if (SemWord.m_GraphDescrs.find("#ПОЛУ") != string::npos)
        N.m_RelOperators.push_back("ПОЛУ");

    if (SemWord.m_GraphDescrs.find("ЭКС-") != string::npos)
        N.m_RelOperators.push_back("ЭКС-");

    if (SemWord.m_GraphDescrs.find("ВИЦЕ-") != string::npos)
        N.m_RelOperators.push_back("ВИЦЕ-");

    if (SemWord.m_GraphDescrs.find("#КАК_МОЖНО") != string::npos) {
        N.m_RelOperators.push_back("КАК_МОЖНО");
        long ParadigmId = m_pData->GetAdverbWith_O_ByAdjective(SemWord.m_ParadigmId, SemWord.GetWord());
        if (ParadigmId != -1) {
            CFormInfo Paradigm;
            bool bRes = m_pData->GetRusLemmatizer()->CreateParadigmFromID(ParadigmId, Paradigm);
            assert (bRes);
            SemWord.m_ParadigmId = ParadigmId;
            SemWord.m_Lemma = convert_to_utf8(Paradigm.GetWordForm(0), morphRussian);
            SemWord.m_Poses = (1 << ADV);
            SemWord.SetFormGrammems(0);
            GramCodes = "";
        };
    };

    /*
        устанавливаем слот m_AdditParadigmId для
        приложений типа "муж-алкоголик"
    */

    if ((SemWord.m_ParadigmId == -1)
        && (SemWord.m_Lemma.find("-") != string::npos)
        && (SemWord.m_Poses == (1 << NOUN))
            ) {
        int i = SemWord.m_Lemma.find("-");
        SemWord.m_ParadigmId = m_pData->GetFirstParadigmId(morphRussian, SemWord.m_Lemma.substr(0, i), 1 << NOUN);
        SemWord.m_AdditParadigmId = m_pData->GetFirstParadigmId(morphRussian, SemWord.m_Lemma.substr(i + 1), 1 << NOUN);
    };


    std::string S = SemWord.GetWord();
    //  создаем интерпретацию для наречий типа "по-английски", "по-человечески"
    if (startswith(S, "ПО-")
        &&
        ((SemWord.m_Poses == (1 << ADV))
         || ((SemWord.m_ParadigmId == -1)
             && (   endswith(S, "КИ"))
                 || endswith(S, "МУ")
             )
         )
        )
    {
        const size_t prefix_len = std::string("ПО-").length();
        if (endswith(S, "КИ")) {
            // "по-солдатски"
            SemWord.m_Lemma = S.substr(prefix_len) + "Й";
            SemWord.m_ParadigmId = m_pData->GetFirstParadigmId(morphRussian, SemWord.m_Lemma, 1 << ADJ_FULL);
        } else {
            // "по-хорошему"
            SemWord.m_ParadigmId = m_pData->GetFirstParadigmIdAndLemma(morphRussian, S.substr(prefix_len), 1 << ADJ_FULL,
                                                                       SemWord.m_Lemma);
        };
        SemWord.m_bAdverbFromAdjective = true;

    };


    // ============================================================
    // =========   строим интерпретации в семантических словарях
    if (!H.m_SimplePrepNos.empty()) {
        for (long l = 0; l < H.m_SimplePrepNos.size(); l++)
            N.AddInterp(CDictUnitInterp(OborRoss, H.m_SimplePrepNos[l]));
    } else {
        /*
        омоним либо является оборотом, либо нет, например, "что"
        */
        if (H.m_OborotNo != -1)
            N.AddInterp(CDictUnitInterp(OborRoss, H.m_OborotNo));
        else
            InitInterps(SemWord, SemWord.HasOneGrammem(rPassiveVoice), ClauseNo, N);
    };


    N.m_bQuoteMarks = (SemWord.m_GraphDescrs.find(" #QUOTED") != string::npos);

    if (SemWord.m_GraphDescrs.find("АВИА") != string::npos)
        N.m_RelOperators.push_back("АВИА");


    // если у краткого прилагательного  нет граммем времени,
    // поставим ему граммему настоящего времени
    if (SemWord.HasPOS(ADJ_SHORT))
        if (!SemWord.HasOneGrammem(rFutureTense) && !SemWord.HasOneGrammem(rPastTense))
            SemWord.SetFormGrammems(SemWord.GetFormGrammems() | _QM(rPresentTense));

    N.m_Words.push_back(SemWord);


    N.SetGrammems(SemWord.GetAllGrammems());
    N.m_GramCodes = GramCodes;
    N.m_SynGroupTypeStr = "";
    N.SetMainWordNo(0);
    N.m_ClauseNo = ClauseNo;
    N.m_TerminId = W.m_iTerminID;
    N.m_ThesaurusId = W.m_ThesType;
    N.m_bFirstInTermin = W.m_bFirstWordInTermin;
    N.m_TypeAncode = TypeAncode;

    N.m_bQuestionWord = N.IsLemmaList(
            {"КТО", "ЧТО", "ГДЕ", "КОГДА", "КУДА", "ОТКУДА", "ЗАЧЕМ", "КАКОЙ",
             "КАК", "ПОЧЕМУ", "ОТЧЕГО", "СКОЛЬКО", "НАСКОЛЬКО"});

    return N;

};


long CRusSemStructure::FindNodeBySyntaxGroupNo(long SyntaxGroupNo) const {
    for (long i = 0; i < m_Nodes.size(); i++)
        if (m_Nodes[i].m_SyntaxGroupNo == SyntaxGroupNo)
            return i;

    return -1;
};


void CRusSemStructure::DelProhibitedInterps(std::string UnitStr, CRusSemNode &N) const {
    MakeUpperUtf8(UnitStr);
    for (long i = 0; i < N.GetInterps().size(); i++) {
        CRossInterpAndLemma M;
        M.m_DictType = N.GetInterps()[i].m_DictType;
        M.m_UnitNo = N.GetInterps()[i].m_UnitNo;
        M.m_Lemma = UnitStr;
        if (_find(m_UserProhibitedLexVars, M)) {
            N.DeleteInterp(i);
            i--;
        };
    };
};

bool CRusSemStructure::HasAuxiliaryPOS(std::string lemma) const {


    std::vector<CFormInfo> ParadigmCollection;
    auto s8 = convert_from_utf8(lemma.c_str(), morphRussian);
    m_pData->GetRusLemmatizer()->CreateParadigmCollection(true, s8, true, false, ParadigmCollection);
    for (auto& p: ParadigmCollection) {
        std::string ancode = p.GetSrcAncode();
        BYTE POS = m_pData->GetRusGramTab()->GetPartOfSpeech(ancode.c_str());
        if ((POS == PRONOUN)
            || (POS == PRONOUN_P)
            || (POS == PREP)
            || (POS == CONJ)
            || (POS == PARTICLE)
            || (POS == INTERJ)
                )
            return true;
    }
    return false;
}

void CRusSemStructure::GetMaxWordWeightByLemma(std::string lemma, CRusSemWord &SemWord) const {
    std::vector<CFormInfo> paradigms;
    auto s8 = convert_from_utf8(lemma.c_str(), morphRussian);
    m_pData->GetRusLemmatizer()->CreateParadigmCollection(true, s8, true, false, paradigms);
    for (auto& p : paradigms) {
        if (SemWord.m_WordWeight < p.GetWordWeight())
            SemWord.m_WordWeight = p.GetWordWeight();
    }
}

CRusSemNode CRusSemStructure::CreateNodeByFirmGroup(const CRelationsIterator *RelIt, long GroupNo) {
    const CGroup &G = RelIt->GetFirmGroups()[GroupNo];
    long ClauseNo = m_piSent->GetPrimitiveClauseNoByClauseNo(m_piSent->m_Words[G.m_iFirstWord].m_iClauseNo);

    CRusSemNode N;

    for (size_t j = G.m_iFirstWord; j <= G.m_iLastWord; j++) {
        const CSynWord &W = m_piSent->m_Words[j];
        std::string Lemma;

        CSynHomonym H(morphRussian);
        H.SetSentence(const_cast<CSentence*> (m_piSent));
        GetHomonym(ClauseNo, j, H);
        Lemma = H.GetLemma();
        CRusSemWord SemWord(j, Lemma);
        SemWord.m_SynWordNo = j;
        SemWord.m_ParadigmId = H.m_lPradigmID;

        SemWord.SetFormGrammems(H.m_iGrammems);
        SemWord.m_GramCodes = H.GetGramCodes();
        SemWord.m_Poses = H.m_iPoses;
        SemWord.m_pData = m_pData;
        {
            //  reading grammems from "type ancode"
            if ((H.m_CommonGramCode.length() > 1) && (H.m_CommonGramCode != "??")) {
                std::string TypeAncode = H.m_CommonGramCode;
                SemWord.SetTypeGrammems(H.m_CommonGramCode.length() > 2 ? H.m_TypeGrammems :
                                        m_pData->GetRusGramTab()->GetAllGrammems(TypeAncode.c_str()));
            };
        }


        SemWord.InitWordFeatures(m_piSent, j);
        if (SemWord.HasOneGrammem(rPerfective))
            SemWord.m_WordEquals = m_pData->GetAspVerb(SemWord.m_ParadigmId, true);
        if (SemWord.HasOneGrammem(rNonPerfective))
            SemWord.m_WordEquals = m_pData->GetAspVerb(SemWord.m_ParadigmId, false);


        if (!HasAuxiliaryPOS(SemWord.m_Lemma))
            if (SemWord.m_ParadigmId != -1) {
                CFormInfo Paradigm;
                bool bRes = m_pData->GetRusLemmatizer()->CreateParadigmFromID(SemWord.m_ParadigmId, Paradigm);
                assert (bRes);
                SemWord.m_WordWeight = Paradigm.GetWordWeight();
            } else
                GetMaxWordWeightByLemma(Lemma, SemWord);


        N.m_Words.push_back(SemWord);
        if (j == G.m_MainWordNo) {
            N.SetMainWordNo(N.m_Words.size() - 1);
            N.m_GramCodes = H.GetGramCodes();
            InitInterps(SemWord, false, ClauseNo, N);
        };

    };


    CSynHomonym H(morphRussian);
    H.SetSentence(const_cast<CSentence*> (m_piSent));
    GetHomonym(ClauseNo, G.m_iFirstWord, H);

    if (H.m_OborotNo != -1)
        N.AddInterp(CDictUnitInterp(OborRoss, H.m_OborotNo));

    N.m_ClauseNo = ClauseNo;
    N.m_SynGroupTypeStr = m_piSent->GetOpt()->GetGroupNameByIndex(G.m_GroupType);
    N.m_SyntaxGroupNo = GroupNo;
    N.SetGrammems(G.GetGrammems());

    return N;
};


void CRusSemStructure::InterpretPrepNouns(long ClauseNo) {
    for (long RelNo = 0; RelNo < m_SynRelations.size();)
        if (m_Nodes[m_SynRelations[RelNo].m_SourceNodeNo].m_ClauseNo != ClauseNo)
            RelNo++;
        else if (m_SynRelations[RelNo].m_SynRelName == "ПГ") {
            long PrepNode = m_SynRelations[RelNo].m_SourceNodeNo;

            long NounNode = m_SynRelations[RelNo].m_TargetNodeNo;

            // в словосчетаниях предлоги  были уже приписаны,  и куда более точнее, чем сы сейчас можем
            if (m_Nodes[NounNode].m_Colloc.m_CollocSource != RossType) {
                m_Nodes[NounNode].m_SynReal.m_Preps.clear();
                for (long i = 0; i < m_Nodes[PrepNode].GetInterps().size(); i++)
                    m_Nodes[NounNode].m_SynReal.m_Preps.push_back(m_Nodes[PrepNode].GetInterps()[i]);
            };

            MoveIncomingSynRelations(PrepNode, NounNode);

            m_SynRelations.erase(m_SynRelations.begin() + RelNo);

            MoveSynRelations(PrepNode, NounNode);

            m_Nodes[NounNode].m_PrepWordNo = m_Nodes[PrepNode].GetMinWordNo();
            // у предлоги могут быть операторы, например "только после предстоящего запуска",
            // где оператор "только" подвешивается к предлогу "после"
            // перенесем эти операторы к главному слову
            m_Nodes[NounNode].m_RelOperators.insert(
                    m_Nodes[NounNode].m_RelOperators.begin(),
                    m_Nodes[PrepNode].m_RelOperators.begin(),
                    m_Nodes[PrepNode].m_RelOperators.end());

            //if(m_Nodes[PrepNode].m_Words.size() == 1 && m_Nodes[PrepNode].m_Words[0].m_Word.size() == 1)
            if (!m_Nodes[PrepNode].HasOneGrammem(rNominativ))
                m_Nodes[NounNode].DeleteGrammems(_QM(rNominativ)); //удаляем им.п.

            if (m_Nodes[PrepNode].GetGrammems() & rAllCases)
                m_Nodes[NounNode].SetGrammems(
                        m_Nodes[NounNode].GetGrammems() & (m_Nodes[PrepNode].GetGrammems() | ~rAllCases));

            if (CanBeDeleted(PrepNode))
                DelNode(PrepNode);
        } else
            RelNo++;


};

/*
Переделывает конструкцию: 
	некоторые из солдат  
в конструкцию
	некоторые солдаты.
*/
void CRusSemStructure::InterpretSelectiveRelations(long ClauseNo) {
    for (long RelNo = 0; RelNo < m_SynRelations.size(); RelNo++)
        if (m_Nodes[m_SynRelations[RelNo].m_SourceNodeNo].m_ClauseNo == ClauseNo)
            if (m_SynRelations[RelNo].m_SynRelName == "ЭЛЕКТ_ИГ") {
                long NumeralNode = m_SynRelations[RelNo].m_SourceNodeNo;
                long NounNode = m_SynRelations[RelNo].m_TargetNodeNo;

                // переносим грамматические характеристики и син. отношения от числительного к   существительному
                m_Nodes[NounNode].DeleteGrammems(rAllCases);
                m_Nodes[NounNode].AddGrammems(m_Nodes[NumeralNode].GetGrammems() & rAllCases);
                m_Nodes[NounNode].m_SynReal = m_Nodes[NumeralNode].m_SynReal;
                m_Nodes[NumeralNode].m_SynReal.m_Preps.clear();

                MoveIncomingSynRelations(NumeralNode, NounNode);

                // реверсируем  отношение
                swap(m_SynRelations[RelNo].m_SourceNodeNo, m_SynRelations[RelNo].m_TargetNodeNo);

            }
};

void CRusSemStructure::InterpretRepeatConj(long ClauseNo) {
    /*
    Пусть W1,W2 - отрезок,который накрывается отсоюзной
    стрелкой. Запрещаем стрелкам исходить из узлов,
    лежащим в этом отрезке, в узлы, которые не лежат в этом отрезке.
    Наоборот, возможно. Поскольку, чтобы построилось
    отношение  MUA, нужно, чтобы сначала подчинилисьчлены MUA, хотя в заключительной структуре и обратного не булет.
    */
    for (long i = 0; i < m_SynRelations.size(); i++)
        if (m_Nodes[m_SynRelations[i].m_SourceNodeNo].m_ClauseNo == ClauseNo)
            if (m_SynRelations[i].m_SynRelName == "ОТСОЮЗ") {
                long Word1 = m_Nodes[m_SynRelations[i].m_SourceNodeNo].m_Words[0].m_SynWordNo;
                long Word2 = m_Nodes[m_SynRelations[i].m_TargetNodeNo].m_Words[0].m_SynWordNo;
                // отсоюзное всегда идет слева направо
                for (long NodeNo = 0; NodeNo < m_Nodes.size(); NodeNo++) {
                    CRusSemNode &N = m_Nodes[NodeNo];
                    long W0 = N.m_Words[0].m_SynWordNo;
                    if ((Word1 < W0) && (W0 < Word2)) {
                        N.m_MinActantWordNo = Word1;
                        N.m_MaxActantWordNo = Word2;
                    };
                };
            };

    /*
    удаление первых частей повторяющихся союзов
    */

    SetNodeToDeleteFalse();
    for (size_t NodeNo = m_Clauses[ClauseNo].m_BeginNodeNo; NodeNo < m_Clauses[ClauseNo].m_EndNodeNo; NodeNo++)
        if ((m_Nodes[NodeNo].m_MNAType != RepeatMNA)
            && !m_Nodes[NodeNo].m_bToDelete
            && CanBeDeleted(NodeNo)
                ) {
            std::vector<long> Rels;
            GetOutcomingSynRelations(NodeNo, Rels);
            std::vector<long> Nodes;
            for (long i = 0; i < Rels.size();)
                if (m_SynRelations[Rels[i]].m_SynRelName == "ОТСОЮЗ") {
                    Nodes.push_back(m_SynRelations[Rels[i]].m_TargetNodeNo);
                    i++;
                } else
                    Rels.erase(Rels.begin() + i);

            if (Nodes.size() == 0) continue;

            sort(Nodes.begin(), Nodes.end(), IsLessByMinWordNo(this));

            // уничтожаем все отношения  ОТСОЮЗ, идущие от текущего узла
            DeleteSynRelations(Rels);
            long ToNode = Nodes[Nodes.size() - 1];
            MoveSynRelations(NodeNo, ToNode);
            m_Nodes[ToNode].m_NodeType = MNA;
            m_Nodes[ToNode].m_MNAType = RepeatMNA;
            // все разрывные союзы находятся в словаре оборотов
            m_Nodes[ToNode].DelAllInterps();


            for (long i = 0; i < m_Nodes[NodeNo].GetInterps().size(); i++)
                if (m_Nodes[NodeNo].GetInterps()[i].m_DictType == OborRoss)
                    m_Nodes[ToNode].AddInterp(m_Nodes[NodeNo].GetInterps()[i]);


            m_Nodes[NodeNo].m_bToDelete = true;
            for (long i = 0; i < Nodes.size() - 1; i++)
                if (CanBeDeleted(Nodes[i]))
                    m_Nodes[Nodes[i]].m_bToDelete = true;


        };

    DelNodesToDelete();
};


// процедура находит частицу "бы" в клаузе, которое не входит в обороты или 
// словосочетания (примитивный узел). Если нашла, то уничтожает ее и  приравнивает флаг m_HasParticleBY в
// m_Clauses[ClauseNo] TRUE.
// вариант "б" чаше всего используется в обоороте "если б", поэтому его сюда не внесли,
// чтобы не путать c примерами типа "класс Б", "пунткт б"
void CRusSemStructure::FindParticleBY(long ClauseNo) {
    for (long i = 0; i < m_Nodes.size(); i++)
        if (m_Nodes[i].IsPrimitive() && CanBeDeleted(i))
            if (IsInClause(i, ClauseNo))
                if ((m_Nodes[i].m_Words[0].m_Lemma == "БЫ")
                    || ((m_Nodes[i].m_Words[0].m_Lemma == "Б")
                        && m_Nodes[i].m_Words[0].m_PostPuncts.empty()
                        && (m_Nodes[i].m_Words[0].m_CharCase == LowLow)
                        && (i > 0)
                        && m_Nodes[i - 1].m_Words[0].m_PostPuncts.empty()
                    )
                        ) {
                    DelNode(i);
                    m_Clauses[ClauseNo].m_HasParticleBY = true;
                    i = -1;
                };

};


void CRusSemStructure::BuildPustychaWithETO(long ClauseNo) {
    if (m_Clauses[ClauseNo].m_ClauseSyntaxTop != -1) return;


    size_t i = 0;
    for (; i < m_Nodes.size(); i++)
        if (m_Nodes[i].m_ClauseNo == ClauseNo)
            if (m_Nodes[i].m_MainWordNo != -1) {
                std::string Q = m_Nodes[i].m_Words[m_Nodes[i].m_MainWordNo].GetWord();
                if ((Q == "ЭТО") || (Q == "ТАМ")) break;
            };

    size_t ETONodeNo = i;

    if (ETONodeNo == m_Nodes.size()) return;

    for (i = FindRightClosestNode(ETONodeNo); i != -1; i = FindRightClosestNode(i))
        if (m_Nodes[i].HasOneGrammem(rNominativ))
            break;

    if (i == -1) return;


    CRusSemNode RusSemNode = CreateCopulNode(ClauseNo);
    long CopulNodeNo = InsertNode(RusSemNode);
    AddRelation(CRusSemRelation(m_Nodes[CopulNodeNo].m_Vals[0], CopulNodeNo, ETONodeNo, ""));
    //AddRelation(CSemRelation(m_Nodes[CopulNodeNo].m_Vals[1], CopulNodeNo, i,  Indifferent, -1));

/* Нужно будет переписать заново 
 m_Nodes[CopulNodeNo].m_Patterns.resize(2);
m_Nodes[CopulNodeNo].m_Patterns[0].m_Valency.m_RelationStr= "F-ACT"; m_Nodes[CopulNodeNo].m_Patterns[0].m_pRossDoc = GetRossHolder(Ross);
 m_Nodes[CopulNodeNo].m_Patterns[0].m_GramCorteges.push_back(GetSubjCortege(GetRossHolder(Ross)));
 m_Nodes[CopulNodeNo].m_Patterns[0].m_GramCorteges[0].Items[0] = -1;
 m_Nodes[CopulNodeNo].m_Patterns[1].m_SourceNo = CopulNodeNo;
 m_Nodes[CopulNodeNo].m_Patterns[1].m_Valency = m_Nodes[CopulNodeNo].m_Vals[1];
 m_Nodes[CopulNodeNo].m_Patterns[1].m_pRossDoc = GetRossHolder(Ross);
 m_Nodes[CopulNodeNo].m_Patterns[1].m_GramCorteges.push_back(GetRossHolder(Ross)->rus_subj_gf);
*/
    m_Clauses[ClauseNo].m_ClauseSyntaxTop = CopulNodeNo;

};



// если статья с номером UnitNo содержит А0(S) в поле SF, то функция выдает номер статьи S, 




void CRusSemStructure::DealInvitatoryMoodBeforeTree(long ClauseNo) {
    long RootNodeNo = GetClauseSyntaxTop(ClauseNo);
    if (RootNodeNo == -1) return;
    if (!m_Nodes[RootNodeNo].IsPrimitive()) return;

    // ищем ДАВАЙ-узел
    int DAVAI_NodeNo = m_Clauses[ClauseNo].m_BeginNodeNo;
    for (; DAVAI_NodeNo < m_Clauses[ClauseNo].m_EndNodeNo; DAVAI_NodeNo++)
        if (m_Nodes[DAVAI_NodeNo].IsPrimitive()) {
            auto w = m_Nodes[DAVAI_NodeNo].m_Words[0].GetWord();
            if (   (w == "ДАВАЙТЕ")
                || (w == "ДАВАЙТЕ-КА")
                || (w == "ДАВАЙ")
                || (w == "ДАВАЙ-КА")
                )
                break;
        }
    // если найден  ДАВАЙ-узел, то синтаксис, скорей всего, его объявил главным словом клаузы, что неправильно.
    // попробуем удалить этот узел на время и поискать заново синтаксическую вершину, если она не будет
    // найдена, то это не пригласительное наклонение.
    if (DAVAI_NodeNo < m_Clauses[ClauseNo].m_EndNodeNo && CanBeDeleted(DAVAI_NodeNo)) {
        m_Nodes[DAVAI_NodeNo].m_bToDelete = true;
        long SaveRoot = m_Clauses[ClauseNo].m_RootWordNo;
        m_Clauses[ClauseNo].m_RootWordNo = -1;
        RootNodeNo = GetClauseSyntaxTop(ClauseNo);
        m_Nodes[DAVAI_NodeNo].m_bToDelete = false;
        m_Clauses[ClauseNo].m_RootWordNo = SaveRoot;
        if (RootNodeNo == -1) return;
        MoveSynRelations(DAVAI_NodeNo, RootNodeNo);
    };


    // конец цикла используется как метка выхода из обработки этого случая

    // третий случаq: мягкая пригласительная конструкция
    for (;;) {
        if (!m_Nodes[RootNodeNo].HasOneGrammem(rFutureTense)) break;

        if (DAVAI_NodeNo == m_Clauses[ClauseNo].m_EndNodeNo) break;
        if (CanBeDeleted(DAVAI_NodeNo)) {
            if (!m_Nodes[RootNodeNo].HasRelOperator("_мягк_пригласит_наклонение"))
                m_Nodes[RootNodeNo].m_RelOperators.push_back("_мягк_пригласит_наклонение");
            MoveSynRelations(DAVAI_NodeNo, RootNodeNo);
            DelNode(DAVAI_NodeNo);
        }

        return;
    };

    // первый случай  пригласительной конструкции
    for (;;) {
        if (!m_Nodes[RootNodeNo].HasOneGrammem(rImperative)) break;


        if (DAVAI_NodeNo == m_Clauses[ClauseNo].m_EndNodeNo) break;
        if (CanBeDeleted(DAVAI_NodeNo)) {
            if (!m_Nodes[RootNodeNo].HasRelOperator("_пригласит_наклонение"))
                m_Nodes[RootNodeNo].m_RelOperators.push_back("_пригласит_наклонение");
            MoveSynRelations(DAVAI_NodeNo, RootNodeNo);
            DelNode(DAVAI_NodeNo);
        }
        return;
    };

    // второй случай  пригласительной конструкции
    for (;;) {
        if (!m_Nodes[RootNodeNo].HasGrammems(_QM(rImperative) | _QM(rFirstPerson))
            && !m_Nodes[RootNodeNo].HasGrammems(_QM(rPlural) | _QM(rPastTense))
                )
            break;
        auto word = m_Nodes[RootNodeNo].m_Words[0].GetWord();
        if (!endswith(word, "-КА") && !endswith(word, "ТЕ"))
            break;
        if (!m_Nodes[RootNodeNo].HasRelOperator("_пригласит_наклонение"))
            m_Nodes[RootNodeNo].m_RelOperators.push_back("_пригласит_наклонение");
        return;
    };


    // четвертый случай  пригласительной конструкции обрабатывается после построения
    // дерева, поскольку существование его зависит от заполнения валентностей и при построении
    // нет формальных показателей

};

void CRusSemStructure::DealInvitatoryMoodAfterTree() {
    for (long ClauseNo = 0; ClauseNo < m_Clauses.size(); ClauseNo++) {
        long RootNodeNo = GetClauseSyntaxTop(ClauseNo);
        if (RootNodeNo == -1) continue;;
        // проверка, что в мягком пригл. наклон заполнена первая валентносто, иначе конвертируем
        //
        if (m_Nodes[RootNodeNo].HasRelOperator("_мягк_пригласит_наклонение")) {
            if (GetRusSubj(RootNodeNo) == -1) {
                m_Nodes[RootNodeNo].DelRelOperator("_мягк_пригласит_наклонение");
                m_Nodes[RootNodeNo].m_RelOperators.push_back("_пригласит_наклонение");
            };
        } else {
            // четвертый случай  пригласительной конструкции
            if (ClauseNo == 0)
                if (m_Nodes[RootNodeNo].HasGrammems(_QM(rFirstPerson) | _QM(rSingular) | _QM(rImperative))
                    || m_Nodes[RootNodeNo].HasGrammems(_QM(rPastTense) | _QM(rPlural))
                        )
                    if (HasRichPOS(RootNodeNo, VERB))
                        if (GetRusSubj(RootNodeNo) == -1)
                            if (HasSemFet(m_Nodes[RootNodeNo], "MOV"))
                                if (GetSubordConjFromTheBegining(ClauseNo) == 0)
                                    m_Nodes[RootNodeNo].m_RelOperators.push_back("_пригласит_наклонение");

            // пятый случай пригласительной конструкции
            if (m_Nodes[RootNodeNo].HasGrammems(_QM(rFirstPerson) | _QM(rSingular) | _QM(rImperative))
                || m_Nodes[RootNodeNo].HasGrammems(_QM(rPastTense) | _QM(rPlural))

                    )
                if (m_Clauses[ClauseNo].m_ClauseRuleNo != -1)
                    if (m_ClauseRules[m_Clauses[ClauseNo].m_ClauseRuleNo].m_Name == "сочинение") {
                        std::vector<long> Rels;
                        GetIncomingClauseRelations(ClauseNo, Rels);
                        if (Rels.size() == 1)
                            if (m_Nodes[m_Relations[Rels[0]].m_SourceNodeNo].HasRelOperator(
                                    "_пригласит_наклонение"))
                                m_Nodes[RootNodeNo].m_RelOperators.push_back("_пригласит_наклонение");
                    };

        };
    };

};


CRusSemNode Create_EST_Node(CRusSemStructure &R, long ClauseNo) {
    const CLemmatizer *P = R.m_pData->GetRusLemmatizer();

    std::vector<CFormInfo> paradigms;
    std::string Est = _R("есть");
    P->CreateParadigmCollection(false, Est, false, false, paradigms);


    assert (!paradigms.empty());
    for (auto& p : paradigms) {
        if (p.GetWordForm(0) == _R("БЫТЬ")) {
            std::string GramCodes = p.GetSrcAncode();
            CRusSemWord SemWord(-1, "БЫТЬ");
            uint64_t dummy = R.m_pData->GetRusGramTab()->GetAllGrammems(GramCodes.c_str());
            SemWord.SetFormGrammems(dummy);
            SemWord.m_Poses = 1 << VERB;
            SemWord.SetWord("ЕСТЬ");
            SemWord.m_CharCase = LowLow;
            SemWord.m_ParadigmId = p.GetParadigmId();
            SemWord.m_IsPunct = false;
            SemWord.m_pData = R.m_pData;
            SemWord.m_ILE = false;
            CRusSemNode N;
            N.m_Words.push_back(SemWord);
            N.SetGrammems(SemWord.GetAllGrammems());
            N.m_GramCodes = GramCodes;
            N.m_SynGroupTypeStr = "";
            N.SetMainWordNo(0);
            R.InitInterps(SemWord, false, ClauseNo, N, false);
            N.m_ClauseNo = ClauseNo;
            return N;
        }
    }
    throw CExpc("cannot find \"быть\" in the Russian Dictionary");
};


// переводит тире в клаузе типа "тире" в глагол "есть"
void CRusSemStructure::ProcessDashClauses() {
    for (long ClauseNo = 0; ClauseNo < m_Clauses.size(); ClauseNo++)
        // "Моя сестра комсомолка"
        if (m_Clauses[ClauseNo].m_ClauseType == DASH_T) {
            bool NodeWasCreated = false;
            for (long NodeNo = m_Clauses[ClauseNo].m_BeginNodeNo; NodeNo < m_Clauses[ClauseNo].m_EndNodeNo; NodeNo++)
                if (m_Nodes[NodeNo].IsOneDash()) {
                    CRusSemNode N = Create_EST_Node(*this, ClauseNo);
                    N.m_Words[0].m_WordNo = m_Nodes[NodeNo].m_Words[0].m_WordNo;
                    m_Nodes[NodeNo] = N;
                    //  ставим глагол "есть" в кавычки, если предыдущее слово  было в кавычках
                    if (NodeNo > 0)
                        if ((m_Nodes[NodeNo - 1].m_ClauseNo == ClauseNo)
                            && m_Nodes[NodeNo - 1].m_bQuoteMarks
                                )
                            m_Nodes[NodeNo].m_bQuoteMarks = true;

                    NodeWasCreated = true;
                };

            // Во фразе "Мир - это сон" уничтожаем "это"
            if (NodeWasCreated)
                for (long NodeNo = m_Clauses[ClauseNo].m_BeginNodeNo;
                     NodeNo < m_Clauses[ClauseNo].m_EndNodeNo; NodeNo++)
                    if (m_Nodes[NodeNo].IsLemma("ЭТО") && CanBeDeleted(NodeNo)) {
                        DelNode(NodeNo);
                        break;
                    };

        } else
            // "кто ты?"
            // "где мои деньги?"
        if (m_Clauses[ClauseNo].m_ClauseType == UnknownSyntaxElement) {
            long NodeNo = GetConjWordFromClause(ClauseNo);
            if (NodeNo == -1) continue;
            // "какие розы!" здесь  не надо восстанавливать связку
            if (m_Nodes[NodeNo].IsLemma("КАКОЙ")) continue;
            // пустыха, которая начинается с "чем", будем считать частью сравн. оборота
            if (m_Nodes[NodeNo].IsPrimitive()) {
                if (m_Nodes[NodeNo].m_Words[0].GetWord() == "ЧЕМ") continue;
            };
            FreeWordNo(m_Nodes[NodeNo].GetMaxWordNo() + 1);
            CRusSemNode N = Create_EST_Node(*this, ClauseNo);
            N.m_Words[0].m_WordNo = m_Nodes[NodeNo].GetMaxWordNo() + 1;
            InsertNode(N);
        };

};

/*
 если в клаузе есть узел САМ и узел  СЕБЯ, тогда удаляем 
 узел САМ
 по письму Маши:
"
 Леша, нужно разобраться с составными рефлексивами: сам себя,
 сам себе, ..., сам о себе и самого себя, самому себе, ... о
  самом себе. Я считаю, что в каждом случае нужно убирать
 "сам".
 "
*/
void CRusSemStructure::ProcessSAM_SEBJA() {
    for (long i = 0; i < m_Nodes.size(); i++)
        if (m_Nodes[i].IsLemma("СЕБЯ"))
            if ((i + 1 < m_Nodes.size())
                && m_Nodes[i + 1].IsLemma("САМ")
                && CanBeDeleted(i + 1)
                    )
                DelNode(i + 1);
            else if ((i > 0)
                     && m_Nodes[i - 1].IsLemma("САМ")
                     && CanBeDeleted(i - 1)
                    ) {
                DelNode(i - 1);
                i--;
            };
};


long CRusSemStructure::GetClauseSyntaxTop(long ClauseNo) const {
    std::vector<long> Nodes;

    // если в клаузе есть только один ModalCopul
    FindIf(ClauseNo, &CRusSemStructure::IsModalCopul, Nodes);
    if (Nodes.size() == 1)
        return Nodes[0];

    // если нет сказуесог
    if (m_Clauses[ClauseNo].m_RootWordNo == -1) {

        // если в клаузе есть только один глагол в личной форме
        FindIf(ClauseNo, &CRusSemStructure::IsFiniteVerb, Nodes);
        if (Nodes.size() == 1)
            return Nodes[0];

        // если в клаузе есть только один инфинитив, то будем считать его подлежащим
        FindIf(ClauseNo, &CRusSemStructure::IsInfinitive, Nodes);
        if (Nodes.size() == 1)
            return Nodes[0];

        // если в клаузе есть только один императив, то будем считать его подлежащим
        FindIf(ClauseNo, &CRusSemStructure::IsImperativeVerb, Nodes);
        if (Nodes.size() == 1)
            return Nodes[0];

        // если в клаузе есть только один императив, то будем считать его подлежащим
        FindIf(ClauseNo, &CRusSemStructure::IsPredicative, Nodes);
        if (Nodes.size() == 1)
            return Nodes[0];

        FindIf(ClauseNo, &CRusSemStructure::IsCompAdj, Nodes);
        if (Nodes.size() == 1)
            return Nodes[0];


    } else {
        if (m_Clauses[ClauseNo].m_RootWordNo == -1) return -1;

        return FindNodeByWordNo(m_Clauses[ClauseNo].m_RootWordNo, ClauseNo);
    }


    return -1;
};

void CRusSemStructure::CreateSemNodesBySynttaxRelations() {
        CRelationsIterator RelIt;
        RelIt.SetSentence(m_piSent);
        for (long i = 0; i < m_Clauses.size(); i++)
        RelIt.AddClauseNoAndVariantNo(m_piSent->m_vectorPrClauseNo[i], m_Clauses[i].m_ClauseVariantNo);

        if (!RelIt.BuildRelations()) {
            throw CExpc("Cannot  build relations");
        }


        // создаем группы
        for (size_t j = 0; j < RelIt.GetFirmGroups().size(); j++) {
            CRusSemNode N = CreateNodeByFirmGroup(&RelIt, j);
            if (N.IsWordContainer())
                InsertNode(N);
        }


        // создаем примитивные узлы
        for (size_t j = 0; j < m_piSent->m_Words.size(); j++) {
            if (FindNodeByWordNo(j) == -1)
                if (!m_piSent->m_Words[j].m_bDeleted) {
                    CRusSemNode N = CreatePrimitiveNode(j);
                    if (N.IsWordContainer())
                        InsertNode(N);
                };
        }

        PrintNodes();


        // создаем отношения
        for (long RelNo = 0; RelNo < RelIt.GetRelations().size(); RelNo++) {
            const CSynOutputRelation &piRel = RelIt.GetRelations()[RelNo];
            std::string RelName = m_piSent->GetOpt()->GetGroupNameByIndex(piRel.m_Relation.type);
            long SourceNodeNo = (piRel.m_iSourceGroup == -1) ? FindNodeByWordNo(piRel.m_Relation.m_iFirstWord)
                                                             : FindNodeBySyntaxGroupNo(piRel.m_iSourceGroup);
            if (SourceNodeNo == -1)
                continue; // пример: было не из чего , а уже не запрыгивал
            long TargetNodeNo = (piRel.m_iTargetGroup == -1) ? FindNodeByWordNo(piRel.m_Relation.m_iLastWord)
                                                             : FindNodeBySyntaxGroupNo(piRel.m_iTargetGroup);
            if (TargetNodeNo == -1)
                continue; // пример: было не из чего , а уже не запрыгивал

            // если отношение межклаузное, тогда добавляем в межклаузные
            if (m_Nodes[SourceNodeNo].m_ClauseNo != m_Nodes[TargetNodeNo].m_ClauseNo)
                m_SynClauseRels.push_back(
                        CSynRelation(m_Nodes[SourceNodeNo].m_ClauseNo, m_Nodes[TargetNodeNo].m_ClauseNo, RelName));
            else
                // иначе - в обычные
            {
                m_SynRelations.push_back(CSynRelation(SourceNodeNo, TargetNodeNo, RelName));

                /*очень частный случай. Работает на фразе " Я знаю это больше тебя", где у "тебя" должен
                появиться родительный падеж, который затирается синтаксисом */

                if (m_SynRelations.back().m_SynRelName == "НАР_ЧИСЛ_СУЩ")
                    if (m_Nodes[m_SynRelations.back().m_TargetNodeNo].IsWordForm("БОЛЬШЕ")
                        || m_Nodes[m_SynRelations.back().m_TargetNodeNo].IsWordForm("МЕНЬШЕ")
                            )
                        m_Nodes[m_SynRelations.back().m_SourceNodeNo].AddOneGrammem(rGenitiv);

                //случай ЧИСЛ_СУЩ
                if (m_SynRelations.back().m_SynRelName.find("ЧИСЛ_СУЩ") != std::string::npos
                    || piRel.m_Relation.type == 25) //NOUN_NUMERAL_APPROX
                {
                    const CRusGramTab *R = (CRusGramTab *) piRel.GetOpt()->GetGramTab();
                    if (piRel.m_Relation.m_GramCodes != "" && m_SynRelations.back().m_SynRelName != "НАР_ЧИСЛ_СУЩ")
                        m_Nodes[TargetNodeNo].m_GramCodes = piRel.m_Relation.m_GramCodes; //ЧИСЛ
                    grammems_mask_t RelGr = piRel.m_Relation.m_iGrammems;
                    if (!(RelGr & rAllGenders))
                        RelGr |= m_Nodes[SourceNodeNo].GetGrammems() & rAllGenders;
                    auto rel_ancodes = R->GetAllGramCodes(NOUN, RelGr, CaseNumberGender0);
                    m_Nodes[SourceNodeNo].m_GramCodes = R->GleicheAncode1(0,
                                                                          R->GramCodes().m_ProductiveNoun,
                                                                          rel_ancodes);
                    LOGV << "NUMBER_NOUN: set gramcodes " << m_Nodes[SourceNodeNo].m_GramCodes <<
                    " relation gramcode = " << rel_ancodes <<
                    " for  " << GetNodeStr1(SourceNodeNo) << " -> " << GetNodeStr1(TargetNodeNo);
                    m_Nodes[SourceNodeNo].SetGrammems(RelGr);
                    m_Nodes[TargetNodeNo].SetGrammems(RelGr);
                }
            };
            // Морфологическую "одушевленнсть" нужно брать от главного слова, поскольку,
            //  например, группа ПРИЛ_СУЩ, пересекает граммемы существительного и прилагательных,
            // а у прилагательного не бывает одушевленности. Поэтому и у всей группы не может быть одушевленности.
            if (piRel.m_Relation.m_iGrammems != 0)
                m_Nodes[SourceNodeNo].AddGrammems(piRel.m_Relation.m_iGrammems | (m_Nodes[SourceNodeNo].GetGrammems() &
                                                                                  (_QM(rAnimative) | _QM(rNonAnimative))));

        };

}

void CRusSemStructure::CreateSimilarInfinitives() {
    for (size_t i = 0; i < m_Nodes.size(); i++)
    {
        if (!m_Nodes[i].IsOneDash())  // тире обрабатывает ProcessDashClauses
            if (m_Nodes[i].IsPrimitive() && CanBeDeleted(i))
                if (m_Nodes[i].m_Words[0].m_IsPunct)
                    if (GetSynRelationsCount(i) == 0) {
                        if (m_Nodes[i].m_Words[0].m_GraphDescrs.find("PLU") != -1) continue;
                        /*
                        Пример 1 "Я хочу уйти домой, прийти в восемь". Синтаксис здесь не собирает
                        однородные инфинитивы. Нужно из запятой сделать оператор однородности.
                        */
                        if ((m_Nodes[i].m_Words[0].GetWord() == ",")
                            && (i + 1 < m_Nodes.size())
                            && (m_Nodes[i].m_ClauseNo == m_Nodes[i + 1].m_ClauseNo)
                            && (m_Nodes[i].GetMinWordNo() + 1 == m_Nodes[i + 1].GetMinWordNo())
                            && (GetIncomingSynRelationsCount(i + 1) == 0)
                            && (i > 0)
                            && (m_Nodes[i - 1].m_ClauseNo == m_Nodes[i].m_ClauseNo)
                            && (m_Nodes[i - 1].GetMinWordNo() + 1 == m_Nodes[i].GetMinWordNo())
                                ) {
                            m_SynRelations.push_back(CSynRelation(i, i + 1, "ОДНОР"));
                            continue;
                        };

                        DelNode(i);
                        i--;
                    };
    }
}

void CRusSemStructure::BuildSemNodesBySyntax() {
    LOGV << "starting BuildSemNodesBySyntax";
    m_SynRelations.clear();
    m_SynClauseRels.clear();
    m_Relations.clear();
    m_LexFuncts.clear();
    m_ThesSemRelations.clear();
    m_DopRelations.clear();

    CreateSemNodesBySynttaxRelations();

    for (size_t i = 0; i < m_Nodes.size(); i++)
        CreateVerbAnalyticalForm(i);

    for (long ClauseNo = 0; ClauseNo < m_Clauses.size(); ClauseNo++)
        if (m_Clauses[ClauseNo].m_BeginNodeNo + 1 < m_Clauses[ClauseNo].m_EndNodeNo)
            if ((m_Nodes[m_Clauses[ClauseNo].m_BeginNodeNo].m_Words[0].m_GraphDescrs.find("OPN") != -1)
                /*
                надо проверить, что последний или
                предпоследний (в  конце может стоять точка) знак закрывающая скобка .
                */
                && ((m_Nodes[m_Clauses[ClauseNo].m_EndNodeNo - 1].m_Words[0].m_GraphDescrs.find("CLS") != -1)
                    || (m_Nodes[m_Clauses[ClauseNo].m_EndNodeNo - 2].m_Words[0].m_GraphDescrs.find("CLS") != -1)
                )
                    )
                m_Clauses[ClauseNo].m_bBracketed = true;

    CreateSimilarInfinitives();

    DeleteEmptyClauses();

    FindMotherLandForPustycha();


    for (size_t i = 0; i < m_Clauses.size(); i++) {
        FindParticleBY(i);
        m_Clauses[i].m_ClauseSyntaxTop = GetClauseSyntaxTop(i);
        m_Clauses[i].m_bHasCHEM = ClauseHasChem(i);
        ZaplataCompar1(i);
    };

    ConvertRusNumeralsToArabic();

    ProcessDashClauses();

    ProcessSAM_SEBJA();

    // установка наклонения (изъявительное по умолчанию)
    m_SentenceMood = Indicative;

    if (!m_piSent->m_Words.empty()) {
        std::string t = m_piSent->m_Words.back().m_strWord;
        if (t == "?")
            m_SentenceMood = Interrogative;
        else if (t == "!")
            m_SentenceMood = Exclamative;
    };

    /*
    у слова СЕБЯ не прописао число, а может быть как мн, так и ед.
    Например, "ты любишь себя", "*они заключили договор между собой"
    */
    for (size_t i = 0; i < m_Nodes.size(); i++)
        if ((m_Nodes[i].IsPrimitive())
            && (m_Nodes[i].m_Words[0].m_Lemma == "СЕБЯ")
                )

            m_Nodes[i].AddGrammems(rAllNumbers);

    for (size_t i = 0; i < m_Nodes.size(); i++)
        m_Nodes[i].m_bCompAdj = IsCompAdj(i) || HasSynRelation(i, "АНАТ_СРАВН");

    /*
    перевешиваем графету КАК_МОЖНО во фразе
    "как можно более простым языком" со слова "более" на "простой",
    */
    for (size_t i = 0; i < m_SynRelations.size(); i++)
        if (m_SynRelations[i].m_SynRelName == "АНАТ_СРАВН")
            if (m_Nodes[m_SynRelations[i].m_TargetNodeNo].HasRelOperator("КАК_МОЖНО") &&
                CanBeDeleted(m_SynRelations[i].m_TargetNodeNo)) {
                m_Nodes[m_SynRelations[i].m_TargetNodeNo].DelRelOperator("КАК_МОЖНО");
                m_Nodes[m_SynRelations[i].m_SourceNodeNo].m_RelOperators.push_back("КАК_МОЖНО");
                DelNode(m_SynRelations[i].m_TargetNodeNo);
            };


    BuildAnalyticalSupelative();

    for (size_t ClauseNo = 0; ClauseNo < m_Clauses.size(); ClauseNo++) {
        m_Clauses[ClauseNo].m_bQuoted = true;
        for (int NodeNo = m_Clauses[ClauseNo].m_BeginNodeNo; NodeNo < m_Clauses[ClauseNo].m_EndNodeNo; NodeNo++)
            m_Clauses[ClauseNo].m_bQuoted = m_Clauses[ClauseNo].m_bQuoted && m_Nodes[NodeNo].m_bQuoteMarks;
    };
    //*

};

