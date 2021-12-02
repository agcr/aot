
//================================
// ==========   Dialing Lemmatizer (www.aot.ru)
//================================
#include "morph_dict/common/utilit.h"
#include "../LemmatizerLib/GraphanAndMorphanHolder.h"


void PrintUsage()
{
		printf ("Dialing File Lemmatizer(www.aot.ru)\n");
		printf ("Usage: FileLem (RUSSIAN|ENGLISH|GERMAN) <input>\n");
		printf ("Example: FileLem Russian input_list.txt\n");
		printf ("<input> is a file with list of files (one file per line)\n");
		printf ("Possible file encodings: Russian(windows-1251), German(windows-1252)\n");
		exit(-1);	
};

int main(int argc, char **argv)
{
	// ===============  LOADING DICTS ================
	if (argc != 3)
		PrintUsage();

	if (		!strcmp (argv[1],  "-h")
			||  !strcmp (argv[1],  "-help")
			||  !strcmp (argv[1],  "/h")
			||  !strcmp (argv[1],  "/help")
	)
		PrintUsage();

	MorphLanguageEnum Language;
	if (!GetLanguageByString(argv[1], Language))
	{
		PrintUsage();
	};
	fprintf (stderr, "Loading dictionaries \n");
	CGraphanAndMorphanHolder Holder;
	Holder.LoadGraphanAndLemmatizer(Language);

	
	const char* FileName = argv[2];
	FILE * fp = fopen (FileName, "r");
	if (!fp)
	{
		printf ("  Cannot open %s \n", FileName );
		return 1;
	};
	char buffer[1024];
	while (fgets(buffer, 1024, fp))
	{
		std::string f = buffer;
		Trim(f);
		int CountOfWords;
		printf ("  Reading %s \n", f.c_str());
		if (!Holder.GetMorphology(f,true, CountOfWords)) 
		{
			printf ("  Cannot process %s \n", f.c_str() );
			continue;
		};
		printf ("  Found %li items\n", Holder.m_PlmLines.m_PlmItems.size() );
		//  morph_dict
		//  save
		std::string LemFile = MakeFName(f, "lem");
		printf ( "  Save to %s\n", LemFile.c_str() );
		Holder.m_PlmLines.SaveToFile(LemFile);
	};


	return 0;
}
