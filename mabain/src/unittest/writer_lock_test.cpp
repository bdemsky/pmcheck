#include <string>
#include <thread>
#include <iostream>

#include <gtest/gtest.h>

#include "../db.h"
#include "../resource_pool.h"

using namespace mabain;

namespace {

#define MB_DIR "/var/tmp/mabain_test/"

class WriterLockTest : public ::testing::Test
{
public:
    WriterLockTest() {
    }
    virtual ~WriterLockTest() {
    }
    virtual void SetUp() {
        std::string cmd = std::string("rm ") + MB_DIR + "_*";
        if(system(cmd.c_str()) != 0) {
        }
    }
    virtual void TearDown() {
        ResourcePool::getInstance().RemoveAll();
    }

protected:
};

TEST_F(WriterLockTest, test_lock)
{
    int options = CONSTS::WriterOptions();
    DB db(MB_DIR, options);
    EXPECT_TRUE(db.is_open());

    DB db1(MB_DIR, options);
    EXPECT_TRUE(db1.Status() == MBError::WRITER_EXIST);

    db.Close();
    DB db2(MB_DIR, options);
    EXPECT_TRUE(db2.is_open());

    options = CONSTS::ReaderOptions();
    DB db3(MB_DIR, options);
    EXPECT_TRUE(db3.is_open());

    DB db4(MB_DIR, options);
    EXPECT_TRUE(db4.is_open());

    db4 = db3;
    EXPECT_TRUE(db4.is_open());

    DB db5(db4);
    EXPECT_TRUE(db5.is_open());
}

// Multiple process lock test
/****
TEST_F(WriterLockTest, test_lock_process)
{
#define LOCK_WRITER_SOURCE " \
#include <unistd.h>\n \
#include \"../db.h\"\n \
using namespace mabain;\n \
int main(int argc, char *argv[]) { \
    int time_to_run = 30; \
    const char *db_dir = \"/var/tmp/mabain_test/\"; \
    if(argc > 1) { \
        db_dir = argv[1]; \
    } \
    if(argc > 2) { \
        time_to_run = atoi(argv[2]); \
    } \
    int options = CONSTS::WriterOptions(); \
    DB db(db_dir, options); \
    if(db.is_open()) { \
        int time_left = time_to_run; \
        while(time_left-- > 0) { \
            sleep(1); \
        } \
    } else { \
        std::cerr << \"failed to open db \" << db_dir << std::endl; \
    } \
    db.Close(); \
    return 0; \
}"

    std::ofstream cpp_source("./lock_writer.cpp");
    cpp_source << LOCK_WRITER_SOURCE; 
    cpp_source.close();
    std::string cmd = "g++ -o lock_writer lock_writer.cpp -L.. -lmabain";
    system(cmd.c_str());
    assert(access("./lock_writer", F_OK) == 0);

    cmd = std::string("./lock_writer ") + MB_DIR + " 60 &";
    system(cmd.c_str());
    sleep(3);
    
    int options = CONSTS::WriterOptions();
    DB db(MB_DIR, options);
    EXPECT_TRUE(db.Status() == MBError::WRITER_EXIST);
    db.Close();

    system("pkill -9 lock_writer");
    sleep(3);
    db = DB(MB_DIR, options);
    EXPECT_TRUE(db.Status() == MBError::SUCCESS);

    unlink("./lock_writer.cpp");
    unlink("./lock_writer");

}
***/

}
