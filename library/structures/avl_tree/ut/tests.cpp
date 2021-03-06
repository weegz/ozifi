#include <gtest/gtest.h>
#include <map>

#include <library/structures/avl_tree/avl_tree.h>

using namespace std;

TEST(avl_tree_simple, setSimple) {
    NAvlTree::set<int> mset;
    mset.insert(10);
    mset.insert(30);
    mset.insert(-3);
    mset.insert(50);
    mset.insert(50); // not inserted
    mset.insert(41);
    NAvlTree::set<int>::iterator it;
    it = mset.begin();

    ASSERT_TRUE(it != mset.end());
    ASSERT_EQ(*it, -3);
    ++it;
    ASSERT_EQ(*it, 10);
    ++it;
    ASSERT_EQ(*it, 30);
    ++it;
    ASSERT_EQ(*it, 41);
    ++it;
    ASSERT_EQ(*it, 50);
    ASSERT_TRUE(it != mset.end());
    ++it;
    ASSERT_TRUE(it == mset.end());
}

TEST(avl_tree_simple, setFind) {
    NAvlTree::set<int> mset;
    mset.insert(10);
    mset.insert(30);
    mset.insert(-3);
    mset.insert(50);
    mset.insert(41);

    NAvlTree::set<int>::iterator it = mset.find(23);
    ASSERT_TRUE(it == mset.end());

    it = mset.find(30);
    ASSERT_TRUE(it != mset.end());
}

TEST(avl_tree_simple, mapSimple) {
    NAvlTree::map<int, int> mmap;
    mmap.insert(std::pair<int, int>(10, 40));
    NAvlTree::map<int, int>::iterator it;
    it = mmap.find(10);
    ASSERT_TRUE(it != mmap.end());
    ASSERT_EQ(it->second, 40);
    ASSERT_EQ(mmap[10], 40);
    mmap[30] = 8;
    ASSERT_EQ(mmap[30], 8);
}

int main(int argc, char **argv) {
    setlocale(LC_CTYPE, "en_US.UTF-8");
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
