#include <map>
#include <unordered_map>
#include <string>

#include <gtest/gtest.h>

#include "RadixTree.h"

using namespace std;

TEST(RadixTree, Create)
{
	RadixTree trie;
	trie.dump();
}

TEST(RadixTree, PutGet)
{
	RadixTree trie;
	trie.put("test", 1);
	trie.dump();
	EXPECT_EQ(1, trie.get("test"));

	// Test overlap
	trie.put("test", 2);
	trie.dump();
	EXPECT_EQ(2, trie.get("test"));

	// longer key than current leaf
	trie.put("test123", 123);
	trie.dump();
	EXPECT_EQ(123, trie.get("test123"));
	EXPECT_EQ(2, trie.get("test"));

	// add one more with exactly matching prefix
	trie.put("test567", 567);
	trie.dump();
	EXPECT_EQ(567, trie.get("test567"));
}

TEST(RadixTree, Remove)
{
	RadixTree t;

	t.put("test", 1);
	t.put("test123", 123);
	t.put("test456", 456);

	EXPECT_EQ(1, t.get("test"));
	EXPECT_EQ(123, t.get("test123"));
	EXPECT_EQ(456, t.get("test456"));

	t.remove("test");
	EXPECT_EQ(0, t.get("test"));
	t.remove("test123");
	EXPECT_EQ(0, t.get("test123"));
	t.remove("test456");
	EXPECT_EQ(0, t.get("test456"));

	t.remove("non-exist");		// does not return error or throw exception
}

int total = 1000000;

TEST(RadixTree, TPS)
{
	RadixTree t;
	
	for (int i=0; i<total; i++) {
		char key[50];
		sprintf(key, "%d", i);
		t.put(key, i);
	}
	for (int i=0; i<total; i++) {
		char key[50];
		sprintf(key, "%d", i);
		EXPECT_EQ(i, t.get(key));
	}
	
	for (int i=0; i<total; i++) {
		char key[50];
		sprintf(key, "%d", i);
		t.remove(key);
	}
}


TEST(Map, TPS)
{
	map<string, int> m;
	
	for (int i=0; i<total; i++) {
		char key[50];
		sprintf(key, "%d", i);
		m[key] = i;
	}
	for (int i=0; i<total; i++) {
		char key[50];
		sprintf(key, "%d", i);
		EXPECT_EQ(i, m[key]);
	}
	
	for (int i=0; i<total; i++) {
		char key[50];
		sprintf(key, "%d", i);
		m.erase(key);
	}
}


TEST(HashMap, TPS)
{
	unordered_map<string, int> m;
	
	for (int i=0; i<total; i++) {
		char key[50];
		sprintf(key, "%d", i);
		m[key] = i;
	}
	for (int i=0; i<total; i++) {
		char key[50];
		sprintf(key, "%d", i);
		EXPECT_EQ(i, m[key]);
	}
	
	for (int i=0; i<total; i++) {
		char key[50];
		sprintf(key, "%d", i);
		m.erase(key);
	}
}

TEST(RadixTree, Traverse)
{
	RadixTree t;

	t.put("test", 1);
	t.put("test123", 123);
	t.put("test567", 567);
	t.put("a", 100);
	t.put("z", 100);
	t.dump();
	
	char key[4096];
	strcpy(key, "");

	printf("============= Begin =============\n");
	while (true) {
		t.next(key, sizeof(key));
		if(strlen(key) == 0) {
			break;
		}
		else {
			printf(" - %s\n", key);
		}
	}
	printf("============== end ==============\n");

}

int main(int argc, char *argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

