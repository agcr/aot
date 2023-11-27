#include "morph_dict/common/utilit.h"
#include "dicts/StructDictLib/Ross.h"
#include "dicts/StructDictLib/TempArticle.h"
#include "morph_dict/common/argparse.h"

#define DOCTEST_CONFIG_IMPLEMENT
#include "morph_dict/contrib/doctest/doctest.h"




TEST_CASE("transliteration") {
	translate_helper t;
	CEngSemWord w;
	w.m_Word = "мама";
	t.transliterate(w);
	CHECK("mama" == w.m_Word);
	w.m_Word = "пуепе";
	w.m_Lemma  = "";
	t.transliterate(w);
	CHECK("puyepe" == w.m_Word);
}



int main(int argc, char** argv) {
	init_plog(plog::Severity::debug, "struct_dict_test.log");
	doctest::Context context;
	context.applyCommandLine(argc, argv);
	int res;
	try {
		res = context.run(); // run doctest
	}
	catch (std::exception& e)
	{
		PLOGE << "test = " << " : " << e.what();
		return 1;
	}
	// important - query flags (and --exit) rely on the user doing this
	if (context.shouldExit()) {
		// propagate the result of the tests
		return res;
	}
}

