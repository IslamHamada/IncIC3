/*********************************************************************
Copyright (c) 2013, Aaron Bradley

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*********************************************************************/

#include <iostream>
#include <string>
#include <time.h>

extern "C" {
#include "aiger.h"
}

#include "IC3.h"
#include "Model.h"

int main(int argc, char **argv) {
    unsigned int propertyIndex = 0;
    bool basic = false, random = false;
    int verbose = 0;
    for (int i = 1; i < argc; ++i) {
        if (string(argv[i]) == "-v")
            // option: verbosity
            verbose = 2;
        else if (string(argv[i]) == "-s")
            // option: print statistics
            verbose = max(1, verbose);
        else if (string(argv[i]) == "-r") {
            // option: randomize the run, which is useful in performance
            // testing; default behavior is deterministic
            srand(time(NULL));
            random = true;
        } else if (string(argv[i]) == "-b")
            // option: use basic generalization
            basic = true;
        else
            // optional argument: set property index
            propertyIndex = (unsigned) atoi(argv[i]);
    }
    string file_name = "/home/islam/Documents/PhD/Model_Checking/IncIC3/IncIC3/my_smv/counter4_2.aag";

    // read AIGER model
    aiger *aig = aiger_init();
//    const char *msg = aiger_read_from_file(aig, stdin);
    const char *msg = aiger_open_and_read_from_file(aig, file_name.c_str());

    if (msg) {
        cout << msg << endl;
        return 0;
    }

    // create the Model from the obtained aig
    Model *model = modelFromAiger(aig, propertyIndex);
    aiger_reset(aig);

    if (!model) return 0;

    bool rv;
    // model check it
    clock_t begin_time = clock();
    IC3::IC3 ic3(*model);
    rv = IC3::check(*model, ic3, verbose, basic, random);
    std::cout << float( clock () - begin_time ) /  CLOCKS_PER_SEC << endl;
    cout << "Constrained Instance" << endl;
    cout << "done" << endl;
    cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
    cout << !rv << endl;

    aiger *aig2 = aiger_init();
    const char *msg2 = aiger_open_and_read_from_file(aig2, file_name.c_str());
    Model *model2 = modelFromAiger(aig2, propertyIndex);
    model2->init.pop_back();
    aiger_reset(aig2);

    begin_time = clock();
    IC3::IC3 ic3_2(*model2, ic3, 1);
    rv = ic3_2.check();
    cout << !rv << endl;
    std::cout << float( clock () - begin_time ) /  CLOCKS_PER_SEC << endl;
    cout << "Relaxed Inc1" << endl;
    cout << "done" << endl;
    cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
    cout << !rv << endl;

    aiger *aig3 = aiger_init();
    const char *msg3 = aiger_open_and_read_from_file(aig3, file_name.c_str());
    Model *model3 = modelFromAiger(aig3, propertyIndex);
    model3->init.pop_back();
    aiger_reset(aig3);

    begin_time = clock();
    IC3::IC3 ic3_3(*model3, ic3, 2);
    rv = ic3_3.check();
    cout << !rv << endl;
    std::cout << float( clock () - begin_time ) /  CLOCKS_PER_SEC << endl;
    cout << "Relaxed Inc2" << endl;
    cout << "done" << endl;
    cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
    cout << !rv << endl;

    aiger *aig4 = aiger_init();
    const char *msg4 = aiger_open_and_read_from_file(aig4, file_name.c_str());
    Model *model4 = modelFromAiger(aig4, propertyIndex);
    model4->init.pop_back();
    aiger_reset(aig4);

    begin_time = clock();
    IC3::IC3 ic3_4(*model4);
    bool rv2 = IC3::check(*model4, ic3_4, verbose, basic, random);
    cout << !rv2 << endl;
    std::cout << float( clock () - begin_time ) /  CLOCKS_PER_SEC << endl;
    cout << "Relaxed Instance" << endl;
    cout << "done" << endl;
    cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << endl;

    // print 0/1 according to AIGER standard
//    cout << !rv2 << endl;

    delete model;
//    delete model2;
//    delete model3;
//    delete model4;

    return 1;
}
