#include <QTest>

#include "project/UnitTest/sequenceitemtest.h"
#include "project/UnitTest/sequencetest.h"

int main(int argc, char** argv)
{
    int status = 0;
    SequenceItemTest sit;
    status |= QTest::qExec(&sit, argc, argv);
    SequenceTest st;
    status |= QTest::qExec(&st, argc, argv);
    return 0;
}
