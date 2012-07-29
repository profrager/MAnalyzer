#include <fstream>
#include <sstream>

#include <cstdlib>
#include <cstring>

#include "analyzer.hpp"

//******************************************************************************
// CREATING/DESTROYING
//******************************************************************************

Analyzer * analyzer_new()
{
    #ifdef ANALYZER_DEBUG
        printf("Analyzer debug mode enabled.\n");
    #endif

    Analyzer * result = new Analyzer;

    // Reading dictionaries from files.
    // Predict prefixes...
    std::ifstream predict_prefixes_file("dics/predict_prefixes.dawgdic", std::ios::binary);
    result -> predict_prefixes.Read(&predict_prefixes_file);

    // ... and lemmas.
    std::ifstream lemmas_file("dics/lemmas.dawgdic", std::ios::binary);
    result -> lemmas.Read(&lemmas_file);

    // Reading lemmas rules (connects each lemma's id to it's rules).
    result -> l_rules = lemmas_rules_fread("dics/lemmas_rules");

    // Reading rules from directory.
    result -> rules = rules_dread("dics/rules");

    return result;
}

void analyzer_free(Analyzer * analyzer)
{
    lemmas_rules_free(analyzer -> l_rules);
    rules_free(analyzer -> rules);
    delete analyzer;
}

//******************************************************************************
// HELPFULL FOR ANALYSIS
//******************************************************************************

// Searches lemmas by word.
// TODO Add buffer for results.
bool analyzer_search_lemmas(Analyzer * analyzer, char * word, int word_size)
{
    bool result = false;

    // Root of dictionary.
    dawgdic::BaseType index = analyzer -> lemmas.root();

    // Prefix matching in the lemmas DAWG dic.
    for(char *q = word; *q != '\0'; q++)
    {
        // Following the transition.
        if(!analyzer -> lemmas.Follow(*q, &index))
            break;

        // If found a lemma, trying to get ids for it.
        if(analyzer -> lemmas.has_value(index))
        {
            int value = analyzer -> lemmas.value(index);
            short int * rules = analyzer -> l_rules.rules[value];
            char * ending = q + 1;
            int ending_len = word_size - ((q + 1) - word);

            // Debug information.
            #ifdef ANALYZER_DEBUG
                // Cut off the begining.
                char old_char = *(q + 1);
                *(q + 1) = '\0';

                // Printing lemma.
                printf("Lemma %s (%d). ", word, value);

                // Restore char.
                *(q + 1) = old_char;

                // Printing ending.
                printf("(Possibly) ending is %s (%d).\n", ending, ending_len);
            #endif

            // Going through all rules for this lemma and check the ending.
            for(int i = 0; i < rules[0]; i++)
            {
                #ifdef ANALYZER_DEBUG
                    printf("\tRule %d\n", rules[i + 1]);
                #endif

                // Searching ending in rule's dawgdic.
                int value;
                if(analyzer -> rules -> dics[rules[i + 1]].Find(ending, ending_len, &value))
                {
                    int count = analyzer -> rules -> forms -> counts[value];
                    FormInfo * forms = analyzer -> rules -> forms -> forms[value];
                    // Check for each prefix and forming result.
                    for(int j = 0; j < count; j++)
                    {
                        result = true;

                        #ifdef ANALYZER_DEBUG
                            printf("\t\tForm %d\n", forms[j].id);
                        #endif

                        #ifdef QUIET_ANALYZER_DEBUG
                            printf("%d ", forms[j].id);
                        #endif
                    }
                }
            }
        }
    }

    #ifdef ANALYZER_DEBUG
        printf("\n");
    #endif

    #ifdef QUIET_ANALYZER_DEBUG
        printf("\n");
    #endif

    return result;
}

// Searches predict prefix for the word.
// TODO Result return?
bool analyzer_search_predict_prefixes(Analyzer * analyzer, char * word)
{
    bool result = false;

    // Root of dictionary.
    dawgdic::BaseType index = analyzer -> lemmas.root();

    // Prefix matching in the predict prefixes DAWG dic.
    for(char *q = word; *q != '\0'; q++)
    {
        // Following a transition.
        if(!analyzer -> predict_prefixes.Follow(*q, &index))
            break;

        // Found a prefix?
        if(analyzer -> predict_prefixes.has_value(index))
        {
            // Debug information.
            #ifdef ANALYZER_DEBUG
                // Cutting off the begining.
                char old_char = *(q + 1);
                *(q + 1) = '\0';

                printf("Predict prefix is %s\n", word);

                // Restoring the char.
                *(q + 1) = old_char;
            #endif

            result = true;
        }
    }

    return result;
}
