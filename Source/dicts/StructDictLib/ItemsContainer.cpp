// ==========  This file is under  LGPL, the GNU Lesser General Public Licence
// ==========  Dialing Structural Dictionary (www.aot.ru)
// ==========  Copyright by Alexey Sokirko (1998-2002)

#pragma warning  (disable : 4530)

#include "StdRoss.h"
#include <algorithm>
#include <stdio.h>
#include "ItemsContainer.h"
#include "LessDomItem.h"
#include "morph_dict/common/util_classes.h"

//=================================================
//=============== CDomen =========================
//==================================================


//==================================================
//==================================================
inline size_t get_size_in_bytes(const TDomItem &t) {
    return get_size_in_bytes(t.m_Data) + get_size_in_bytes(t.m_DomNo);

};

inline size_t save_to_bytes(const TDomItem &i, BYTE *buf) {
    buf += save_to_bytes(i.m_Data, buf);
    buf += save_to_bytes(i.m_DomNo, buf);
    return get_size_in_bytes(i);
}

inline size_t restore_from_bytes(TDomItem &i, const BYTE *buf) {
    buf += restore_from_bytes(i.m_Data, buf);
    buf += restore_from_bytes(i.m_DomNo, buf);
    return get_size_in_bytes(i);
}


TItemContainer::TItemContainer() {
    m_Language = morphRussian;
};

const char*	TItemContainer::GetDomItemStr(const TDomItem& Item) const
{
    assert (!m_Domens[Item.GetDomNo()].IsFreedDomain());
    return m_Domens[Item.GetDomNo()].GetDomainItemsBuffer() + Item.GetItemStrNo();
};

BYTE TItemContainer::GetDomenNoByDomStr(const char *DomStr) const {
    for (BYTE i = 0; i < m_Domens.size(); i++)
        if (m_Domens[i].GetDomStr() == DomStr)
            return i;

    return ErrUChar;
};

bool TItemContainer::AreEqualDomItems(const TDomItem &Item1, const TDomNoItemStr &Item2) const {
    return    (Item2.ItemStr == GetDomItemStr(Item1))
           && (Item1.GetDomNo() == Item2.DomNo);
}


inline int TItemContainer::GetItemNoByItemStr(const std::string& ItemStr, const char *DomStr) const {
    BYTE DomNo = GetDomenNoByDomStr(DomStr);
    if (DomNo == ErrUChar) return -1;
    return GetItemNoByItemStr(ItemStr, DomNo);
};

int TItemContainer::GetItemNoByItemStr(const std::string& ItemStr, BYTE DomNo) const {
    if (DomNo == ErrUChar) return -1;

    if (DomNo == LexPlusDomNo) {
        DomNo = GetDomNoForLePlus(ItemStr.c_str());
        if (DomNo == ErrUChar) {
            return -1;
        };
    };

    TDomNoItemStr I = { ItemStr, DomNo };
    const CDomen &D = m_Domens[I.DomNo];
    if (D.GetDomainSource() == dsUnion) {
        int Res = -1;

        for (auto& dom_no: D.GetParts()) {
            Res = GetItemNoByItemStr(ItemStr, dom_no);
            if (Res != -1) break;
        }

        return Res;
    } else {
        if (D.DomainIsDelim())
            if (   (ItemStr.length() != 1)
                || (D.GetDomainItemsBuffer() == 0)
                || !strchr(D.GetDomainItemsBuffer(), ItemStr[0])
               )
                return -1;
        auto U = lower_bound(m_DomItems.begin(), m_DomItems.end(), I,
                                                         IsLessByNotStableItemStrNew(this));
        if ((U == m_DomItems.end())
            || !AreEqualDomItems(*U, I)
                )
            return -1;
        else
            return (int) (U - m_DomItems.begin());
    };
}


bool TItemContainer::InitDomensConsts() {

    ActantsDomNo = GetDomenNoByDomStr("D_ACTANTS");
    if (ActantsDomNo == ErrUChar) {
        throw CExpc("cannot find domain D_ACTANTS");
    }

    LexDomNo = GetDomenNoByDomStr("D_RLE");
    if (LexDomNo == ErrUChar) return false;

    LexPlusDomNo = GetDomenNoByDomStr("D_RLE_PLUS");
    if (LexPlusDomNo == ErrUChar) return false;

    IntegerDomNo = GetDomenNoByDomStr("D_INTEGER");

    TitleDomNo = GetDomenNoByDomStr("D_TITLE");


    CollocDomNo = GetDomenNoByDomStr("D_COLLOC");
    if (CollocDomNo == ErrUChar) return false;

    AbbrDomNo = GetDomenNoByDomStr("D_ABBR");
    if (AbbrDomNo == ErrUChar) return false;

    FieldDomNo = GetDomenNoByDomStr("D_FIELDS");
    if (FieldDomNo == ErrUChar) return false;

    EmptyDomNo = GetDomenNoByDomStr("D_");
    if (EmptyDomNo == ErrUChar) return false;

    return true;
};


//  перестраивает все константые домены (те, которые могут меняться только 
// из редактора схемы. Константые домены - это просто не текстовые домены (CDomen::Source != dsText).
void TItemContainer::UpdateConstDomens() {
    for (size_t i = 0; i < m_Domens.size(); i++) {
        CDomen &D = m_Domens[i];

        if (i == EmptyDomNo)
            for (int k = D.m_StartDomItem; k < D.m_EndDomItem; k++)
                if (m_DomItems[k].GetItemStrLen() > 0)
                    WildCardDomItemNo = k;

    };

};


bool TItemContainer::BuildDomens(char *LastReadLine) {
    std::ifstream inp;
    inp.open(DomensFile);
    if (!inp.good()) {
        throw CExpc("cannot open file %s", DomensFile.c_str());
    }
    auto domains = nlohmann::json::parse(inp);
    std::unordered_map<std::string, BYTE>  doms_idents;
    for (auto d: domains) {
        size_t dom_no = m_Domens.size();
        CDomen T;
        T.ReadFromJson(this, dom_no, d);
        m_Domens.emplace_back(T);
        doms_idents[T.GetDomStr()] = dom_no;
    }
    for (auto& d : m_Domens) {
        d.InitDomainParts(doms_idents);
    }
   
    
    return InitDomensConsts();
}


bool TItemContainer::BuildDomItems() {

    m_DomItems.clear();

    //  reading domen items
    {
        FILE *fp = fopen(DomItemsTextFile.c_str(), "rb");
        if (!fp) return false;
        int i1, i2;
        while (fscanf(fp, "%i %i\n", &i1, &i2) == 2) {
            TDomItem I;
            I.m_Data = i1;
            I.m_DomNo = i2;
            if (i2 >= 255) return false;
            m_DomItems.push_back(I);

        }
        fclose(fp);
    }

    //  reading domens
    {
        FILE *fp = fopen(ItemsFile.c_str(), "r");
        if (!fp) return false;

        for (size_t k = 0; k < m_Domens.size(); k++) {
            char buffer[513];
            /*
                при добавлении новых доменов файл Items получается  неполным
                (там нет добавленных доменов), поэтому  следующий fgets
                на добавленном домене вернет false
            */
            if (!fgets(buffer, 512, fp)) break;
            //std::string q = convert_from_utf(buffer, m_Language);
            std::string q = buffer;
            StringTokenizer tok(q.c_str(), ";");
            if (!tok()) return false;
            assert (tok.val() == m_Domens[k].GetDomStr());
            if (tok.val() != m_Domens[k].GetDomStr())
                return false;

            m_Domens[k].m_DomainItemsBufferLength = tok() ? atoi(tok.val()) : 0;
            if (m_Domens[k].m_DomainItemsBufferLength == 0)
                m_Domens[k].m_DomainItemsBuffer = 0;
            else {
                m_Domens[k].m_DomainItemsBuffer = (char *) malloc(m_Domens[k].m_DomainItemsBufferLength);
                fread(m_Domens[k].m_DomainItemsBuffer, 1, m_Domens[k].m_DomainItemsBufferLength, fp);
            };
            fgets(buffer, 512, fp);
        };
        fclose(fp);
    }

    //  initializing domens
    for (int i = 0; i < m_DomItems.size(); i++) {
        CDomen &D = m_Domens[m_DomItems[i].GetDomNo()];
        if (D.m_StartDomItem == -1)
            D.m_StartDomItem = i;
        if (i + 1 > D.m_EndDomItem)
            D.m_EndDomItem = i + 1;
    };

    /*
       free example domens, m_bDontLoadExamples is switched on
    */
    if (m_bDontLoadExamples) {
        for (auto& d: m_Domens) {
            if (d.GetDomStr() == "D_EXM" || d.GetDomStr() == "D_THES") {
                d.MakeFree();
            };
        }
    };
    UpdateConstDomens();

    return true;
}

bool TItemContainer::WriteDomItems() const {
    FILE *fp = fopen(DomItemsTextFile.c_str(), "wb");
    for (size_t i = 0; i < m_DomItems.size(); i++) {
        fprintf(fp, "%i %i\n", m_DomItems[i].m_Data, m_DomItems[i].m_DomNo);
    };
    fclose(fp);


    fp = fopen(ItemsFile.c_str(), "wb");
    for (size_t k = 0; k < m_Domens.size(); k++) {
        fprintf(fp, "%s;%i\n",
                m_Domens[k].GetDomStr().c_str(),
                m_Domens[k].m_DomainItemsBufferLength);
        fwrite(m_Domens[k].m_DomainItemsBuffer, 1, m_Domens[k].m_DomainItemsBufferLength, fp);
        fprintf(fp, "\n");
    };
    fclose(fp);


    return true;
};


BYTE TItemContainer::GetDomNoForLePlus(const char *s) const {
    int Number = atoi(s);
    if (Number
        || (!s && (strlen(s) == 1) && (s[0] == '0'))
            )
        return IntegerDomNo;
    else if (IsStandardRusLexeme(s))
        return LexDomNo;
    else if (CanBeRusAbbr(s))
        return AbbrDomNo;
    else if (CanBeRusColloc(s))
        return CollocDomNo;
    else if (!strncmp(s, "D_", 2))
        return LexPlusDomNo;
    else
        return ErrUChar;
};


// строка:
// 1. Cодержащая  только кириллицу;
// 2. Может включать в себя только один дефис;
// 3. Может начинаться с большой буквы, но все остальные буквы должны быть
//    маленькими. 
bool TItemContainer::IsStandardRusLexeme(const char *s) const {
    bool FlagHasHyphen = false;

    if ((strlen(s) == 0)
        || !is_russian_alpha((unsigned char) s[0]))
        return false;

    for (int i = 1; i < strlen(s); i++)
        if (((unsigned char) s[i]) == '-')
            if (FlagHasHyphen)
                return false;
            else
                FlagHasHyphen = true;
        else
/* вообще-то в словаре леммы и словоформы  принято писять маленькими буквами,
но отдельные плохие люди пишут большими, поэтому приходится проверять и те, и другие */
        if (!is_russian_alpha((unsigned char) s[i]))
            return false;

    return ((unsigned char) s[strlen(s) - 1]) != '-';
};


inline bool TItemContainer::CanBeRusAbbr(const char *s) const {

    if (strlen(s) == 1)
        return is_lower_alpha((unsigned char) s[0], m_Language);


    if (strlen(s) == 0)
        return false;

    bool HasUpperAlphaInside = false;

    for (int i = 0; i < strlen(s) - 1; i++) {
        if (!is_russian_alpha((unsigned char) s[i]) && !islower((unsigned char) s[i]) &&
            !strchr("-.//", (unsigned char) s[i]))
            return false;

        HasUpperAlphaInside |= (is_russian_upper((unsigned char) s[i])
                                || (((unsigned char) s[i]) == '/'))
                               && (i > 0);

    };

    return HasUpperAlphaInside || (((unsigned char) s[strlen(s) - 1]) == '.');
}


inline bool TItemContainer::CanBeRusColloc(const char *s) const {
    if ((strlen(s) < 4)
        || (!strchr(s, ' ')
            && !strchr(s, ':')
        )
            )
        return false;

    for (int i = 0; i < strlen(s); i++)
        if (!is_russian_alpha((unsigned char) s[i])
            && !isdigit((unsigned char) s[i])
            && !strchr("-/,\\$:;.|()\"~ ", (unsigned char) s[i])
                )
            return false;

    return true;
};


BYTE TItemContainer::GetFieldNoByFieldStrInner(const char *FieldStr) const {
    BYTE k = 0;


    for (; k < Fields.size(); k++)
        if (!strcmp(Fields[k].FieldStr, FieldStr))
            break;

    if (k == Fields.size())
        return ErrUChar;
    else
        return k;

}

bool TItemContainer::ClearFields() {
    for (size_t i = 0; i < Fields.size(); i++)
        Fields[i].m_Signats.clear();

    Fields.clear();
    return true;
}


bool TItemContainer::BuildFields(BYTE MaxNumDom) {
    ClearFields();
    FILE *fp = fopen(FieldsFile.c_str(), "rb");
    int FieldsCount;
    fscanf(fp, "%u\n", &FieldsCount);
    Fields.resize(FieldsCount);

    if (FieldsCount >= ErrUChar) {
        fclose(fp);
        return false;
    };
    BYTE FieldNo = 0;
    try {
        for (FieldNo = 0; FieldNo < FieldsCount; FieldNo++) {

            char s[255];
            char q[30];

            int SignatCount;
            fgets(s, 255, fp);
            if (sscanf(s, "%u;%u;%[^;];%c;%[^;];%u\n",
                       &Fields[FieldNo].FieldId,
                       &SignatCount,
                       Fields[FieldNo].FieldStr,
                       &Fields[FieldNo].TypeRes,
                       q,
                       &Fields[FieldNo].OrderId) != 6) {
                fclose(fp);
                m_LastError = Format("Cannot read file %s line: %s", FieldsFile.c_str(), s);
                return false;
            };
            Fields[FieldNo].IsApplicToActant = strcmp(q, "FALSE") != 0;


            //printf ("%s\n", Fields[FieldNo].FieldStr);


            for (BYTE i = 0; i < SignatCount; i++) {
                fgets(s, 255, fp);
                CSignat Signat;
                assert (strlen(s) > 0);
                rtrim(s);
                StringTokenizer tok(s, ";");
                tok();
                Signat.SignatId = atoi(tok.val());
                tok();
                Signat.OrderNo = atoi(tok.val());
                tok();
                assert (strlen(tok.val()) < 255);
                strcpy(Signat.FormatStr, tok.val());
                tok();
                assert (strlen(tok.val()) < 255);
                strcpy(Signat.FormatName, tok.val());
                Fields[FieldNo].m_Signats.push_back(Signat);
            };

            fgets(s, 255, fp);

        };
        fclose(fp);

    }
    catch (...) {
        //printf ("errror!!!\n");
        m_LastError = std::string("Error in field ") + std::string(Fields[FieldNo].FieldStr);
        return false;
    };


    return BuildFormats(MaxNumDom);
}

/* todo: use more json serialization

void TItemContainer::WriteFieldsJson() const {
    auto fields = nlohmann::json::array();
    for (auto f : Fields) {
        nlohmann::json signats = nlohmann::json::array();
        for (auto s : f.m_Signats) {
            nlohmann::json signat = {
                {"id", s.SignatId},
                {"order", s.OrderNo},
                {"format", s.FormatStr},
                {"name", s.FormatName},
            };
            signats.push_back(signat);
        }

        nlohmann::json field = {
            {"id", f.FieldId},
            {"name", f.FieldStr},
            {"type", f.TypeRes},
            {"actant", f.IsApplicToActant},
            {"order", f.OrderId},
            {"signats", signats},
        };
        fields.push_back(field);

    };
    std::ofstream outf(FieldsFile);
    outf << fields.dump(4);
    outf.close();

};
*/
bool TItemContainer::WriteFields() const {
    FILE *fp = fopen(FieldsFile.c_str(), "wb");
    fprintf(fp, "%lu\n", Fields.size());
    for (size_t i = 0; i < Fields.size(); i++) {
        fprintf(fp, "%i;%zu;%s;%c;%s;%i\n",
                Fields[i].FieldId,
                Fields[i].m_Signats.size(),
                Fields[i].FieldStr,
                Fields[i].TypeRes,
                Fields[i].IsApplicToActant ? "TRUE" : "FALSE",
                Fields[i].OrderId);
        for (size_t k = 0; k < Fields[i].m_Signats.size(); k++)
            fprintf(fp, "%i;%i;%s;%s\n", Fields[i].m_Signats[k].SignatId, Fields[i].m_Signats[k].OrderNo,
                    Fields[i].m_Signats[k].FormatStr, Fields[i].m_Signats[k].FormatName);
        fprintf(fp, "\n");
    };
    fclose(fp);
    return true;
};


bool TItemContainer::BuildFormats(BYTE MaxNumDom) {
    BYTE FieldNo = 0;
    for (FieldNo = 0; FieldNo < Fields.size(); FieldNo++) {
        std::vector<CSignat> Signats = Fields[FieldNo].m_Signats;

        sort(Signats.begin(), Signats.end());

        BYTE k = 0;

        for (; k < Fields[FieldNo].m_Signats.size(); k++)
            if ((k + 1 < Fields[FieldNo].m_Signats.size())
                && (Signats[k].OrderNo == Signats[k + 1].OrderNo)) {
                m_LastError = "Signats must have unique OrderNo; Field = ";
                m_LastError += Fields[FieldNo].FieldStr;
                return false;
            };

        for (k = 0; k < Fields[FieldNo].m_Signats.size(); k++)
            if (!BuildOneFieldFormat(Fields[FieldNo].m_Signats[k], Fields[FieldNo].m_Signats[k].FormatStr,
                                     Fields[FieldNo].FieldStr, MaxNumDom))
                return false;;

        if (!UpdateSignatsOfTheFieldInCorteges(FieldNo, Signats)) {
            m_LastError = "удален формат, который присут. в словаре; Field = ";
            m_LastError += Fields[FieldNo].FieldStr;
            return false;
        };


        sort(Fields[FieldNo].m_Signats.begin(), Fields[FieldNo].m_Signats.end());
    };

    return true;
};


bool TItemContainer::BuildOneFieldFormat(CSignat &Sgn, char *Frmt, char *Name, BYTE MaxNumDom) {
    char DomStr[100];
    char _Frmt[255];
    strcpy(_Frmt, Frmt);
    Sgn.sFrmt[0] = 0;
    Sgn.sFrmtWithotSpaces[0] = 0;
    Sgn.DomsWithDelims.clear();
    Sgn.Doms.clear();
    if (_Frmt[0] == 0) {
        m_LastError = Format("В формате поля  %s оставлены пустые строки!", Name);
        return false;
    };

    // FunRec.Doms. В конце переменная DomMeanNum должна содержать число
    // значимых доменов.
    StringTokenizer tok(_Frmt, " ");

    while (tok()) {
        strcpy(DomStr, tok.val());
        bool IsMult = (strlen(DomStr) > 0)
                      && ((unsigned char) DomStr[strlen(DomStr) - 1] == '+');
        if (IsMult)
            DomStr[strlen(DomStr) - 1] = 0;

        BYTE DomenNo = GetDomenNoByDomStr(DomStr);
        if (DomenNo == ErrUChar) {
            m_LastError = Format("Домен %s  не объявлен в таблице доменов (формат функции %s)!", DomStr, Name);
            return false;
        };
        if (m_Domens[DomenNo].GetDomainSource() == dsSystem) {
            m_LastError = Format(
                    "Домен %s  объявлен как системный. Такие домены нельзя использовать в сигнатурах(формат функции %s)!",
                    DomStr, Name);
            return false;
        };

        Sgn.DomsWithDelims.push_back(TSignatItem(DomenNo, IsMult));
        if (m_Domens[DomenNo].DomainIsDelim()) {
            strcat(Sgn.sFrmt, " ");
            if (!m_Domens[DomenNo].IsEmpty()) {
                strcat(Sgn.sFrmt, m_Domens[DomenNo].m_DomainItemsBuffer);
                strcat(Sgn.sFrmtWithotSpaces, m_Domens[DomenNo].m_DomainItemsBuffer);
            }
        } else {
            strcat(Sgn.sFrmt, " %s");
            strcat(Sgn.sFrmtWithotSpaces, "%s");


            if (Sgn.Doms.size() == MaxNumDom) {
                m_LastError = Format("Функция %s   Число значимых доменов  не может превышать %i!", Name, MaxNumDom);
                return false;
            }
            if (IsMult) {
                DomenNo = GetDomenNoByDomStr("D_MULT");
                if (DomenNo == ErrUChar) {
                    m_LastError = Format(
                            "Домен D_MULT  не объявлен в таблице доменов, а функции %s используется оператор повтора(+)!",
                            DomStr, Name);
                    return false;
                };
            };

            Sgn.Doms.push_back(DomenNo);

        };

    };

    return true;
};

bool TItemContainer::WriteDomens() const {
    std::ofstream outf(DomensFile);
    if (!outf.is_open())
    {
        throw CExpc("Cannot write to %s", DomensFile.c_str());
    };
    auto domains = nlohmann::json::array();
    for (auto& d : m_Domens) {
        domains.push_back(d.WriteToJson());
    }
    outf << domains.dump(4);
    outf.close();
    return true;
}


void TItemContainer::ErrorMessage(std::string s) const {
    ::ErrorMessage(RossPath, s);
};
