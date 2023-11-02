// ==========  This file is under  LGPL, the GNU Lesser General Public Licence
// ==========  Dialing Posmorphological Module (www.aot.ru)
// ==========  Copyright by Dmitry Pankratov, Alexey Sokirko (1999-2002)

#include "MAPostMain.h"
#include "morph_dict/agramtab/agramtab_.h"

CPostMorphInteface *NewRussianPostMorph(const CLemmatizer *RusLemmatizer, const CAgramtab *RusGramTab) {
    CMAPost *M = new CMAPost;
    if (!M) return 0;
    if (!M->Init(RusLemmatizer, RusGramTab)) {

        delete M;
        return 0;
    };
    return M;
}

CMAPost::CMAPost() {
    m_Language = morphRussian;
    m_bCanChangeInputText = true;
    m_bHumanFriendlyOutput = false;
}

CMAPost::~CMAPost() {
    m_Words.clear();
    m_pRusLemmatizer = 0;

}

CFixedColloc::CFixedColloc(std::string LemmaStr, long MainWordNo, std::string InterfaceString,
                           const CAgramtab *m_piRusGramTab) {
    m_LemmaStr = LemmaStr;
    m_MainWordNo = MainWordNo;
    m_InterfaceString = InterfaceString;
    StringTokenizer tok(LemmaStr.c_str(), " ");
    while (tok()) {
        CLemmaAndPOS L;
        std::string Item = tok.val();
        int i = Item.find("[");
        if (i != std::string::npos) {
            int k = Item.find("]");
            std::string POS = Item.substr(i + 1, k - i - 1);
            uint64_t dummy;
            m_piRusGramTab->ProcessPOSAndGrammems(POS.c_str(), L.m_POS, dummy);
            L.m_Lemma = Item.substr(0, i);
        } else {
            L.m_Lemma = Item;
            L.m_POS = 255;
        };
        m_Lemmas.push_back(L);
    };
    assert (m_MainWordNo < m_Lemmas.size());
    assert (m_MainWordNo >= 0);
};

bool CMAPost::ReadCollocs() {
    try {

        std::string FileName = GetRegistryString("Software\\Dialing\\Mapost\\Collocs");
        FILE *fp = fopen(FileName.c_str(), "rb");
        if (!fp) {
            ErrorMessage("Cannot Load Collocations in Mapost");
            return false;
        };
        char s[1000];
        while (fgets(s, 1000, fp)) {
            std::string LemmaStr;;
            int MainWordNo;
            std::string InterfaceString;
            StringTokenizer tok(convert_from_utf8(s, m_Language).c_str(), "#");
            int i = 0;
            while (tok()) {
                std::string q = tok.val();
                Trim(q);
                if (i == 0)
                    LemmaStr = q;
                else if (i == 1)
                    MainWordNo = atoi(q.c_str());
                else if (i == 2)
                    InterfaceString = q;
                i++;
            };

            m_FixedCollocs.push_back(CFixedColloc(LemmaStr, MainWordNo, InterfaceString, m_pRusGramTab));
        };
        fclose(fp);
    }
    catch (...) {
        return false;
    }
    return true;
};


unsigned int count_of_bits(grammems_mask_t n)
{
    unsigned int count = 0;
    while (n) {
        count += n & 1;
        n >>= 1;
    }
    return count;
}

bool CMAPost::Init(const CLemmatizer *RusLemmatizer, const CAgramtab *RusGramTab) {
    try {
        m_pRusGramTab = RusGramTab;
        m_pRusLemmatizer = RusLemmatizer;
        if (!ReadCollocs()) {
            return false;
        };


        m_DURNOVOGramCode = m_pRusGramTab->GetPlugNouInfo().m_GramCode;
        auto codes = m_pRusGramTab->GetAllGramCodes(NOUN, _QM(rIndeclinable) | _QM(rInitialism), GrammemsInclusion);
        for (size_t i = 0; i < codes.length(); i += 2) {
            m_AbbrIndeclGramCodes.insert(codes.substr(i, 2));
        }
        assert (m_AbbrIndeclGramCodes.size() == 6);

        m_NumWithGendersGramCodes = m_pRusGramTab->GetAllGramCodes(NUMERAL, 0, AnyGender);
        assert (m_NumWithGendersGramCodes.length() == 18 * 2);

        // todo: move it to Russian agramtab library
        m_ProductiveNounGramCodes = "";
        for (uint16_t i = 0; i < m_pRusGramTab->GetMaxGrmCount(); i++) {
            auto *l = m_pRusGramTab->GetLine(i);
            if (l == nullptr) continue;
            if ((l->m_PartOfSpeech == NOUN) && (count_of_bits(l->m_Grammems) == 3)) {
                if ((l->m_Grammems & rAllGenders) && (l->m_Grammems & rAllCases) && (l->m_Grammems & rAllNumbers)) {
                    if ((l->m_Grammems & _QM(rVocativ)) == 0) {
                        std::string ancode = m_pRusGramTab->LineIndexToGramcode(i);
                        m_ProductiveNounGramCodes += ancode;
                        //LOGV << "productive noun  " << m_pRusGramTab->GetTabStringByGramCode(ancode.c_str());
                    }
                }

            }
        }
        assert (m_ProductiveNounGramCodes.length() == 72); //72 = 6 cases * 2 numbers * 3 genders * 2 chars
    }
    catch(CExpc c)
    {
        ErrorMessage("MAPOST", Format("%s", c.m_strCause.c_str()).c_str());
        return false;
    }
    catch(...)
    {
        LOGE << "unknown exception in mapost";
        return false;
    }
    return true;
}

