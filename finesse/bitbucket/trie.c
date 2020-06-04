#include <stdio.h>
#include <stdlib.h>
#include "bitbucket.h"
#include "trie.h"

// Source: https://www.techiedelight.com/trie-implementation-insert-search-delete
// Note that this is a simple, non-synchronized Trie implementation in C.  Sufficient for
// an in-memory file system.
//

// define character size
#define CHAR_SIZE MAX_FILE_NAME_SIZE

// A Trie node
struct Trie
{
	struct  Trie* character[CHAR_SIZE];
    void   *Object; // this is the object to which this Trie entry points
};

// Function that returns a new Trie node
struct Trie* TrieCreateNode(void)
{
	struct Trie* node = (struct Trie*)malloc(sizeof(struct Trie));
    node->Object = NULL;

	for (int i = 0; i < CHAR_SIZE; i++)
		node->character[i] = NULL;

	return node;
}

// Iterative function to insert a string in Trie
void TrieInsert(struct Trie *head, const char* str, void *object)
{
	// start from root node
	struct Trie* curr = head;
	while (*str)
	{
		// create a new node if path doesn't exists
		if (curr->character[*str - 'a'] == NULL)
			curr->character[*str - 'a'] = TrieCreateNode();

		// go to next node
		curr = curr->character[*str - 'a'];

		// move to next character
		str++;
	}

	// mark current node as leaf
    curr->Object = object;
}

// Iterative function to search a string in Trie. It returns 1
// if the string is found in the Trie, else it returns 0
void *TrieSearch(struct Trie* head, const char* str)
{
	// return 0 if Trie is empty
	if (head == NULL)
		return 0;

	struct Trie* curr = head;
	while (*str)
	{
		// go to next node
		curr = curr->character[*str - 'a'];

		// if string is invalid (reached end of path in Trie)
		if (curr == NULL)
			return 0;

		// move to next character
		str++;
	}

	// if current node is a leaf and we have reached the
	// end of the string, return 1
    return curr->Object;
}

// returns 1 if given node has any children
static int haveChildren(struct Trie* curr)
{
	for (int i = 0; i < CHAR_SIZE; i++)
		if (curr->character[i])
			return 1;	// child found

	return 0;
}

// Recursive function to delete a string in Trie
int TrieDeletion(struct Trie **curr, const char* str)
{
	// return if Trie is empty
	if (*curr == NULL)
		return 0;

	// if we have not reached the end of the string
	if (*str)
	{
		// recur for the node corresponding to next character in
		// the string and if it returns 1, delete current node
		// (if it is non-leaf)
		if ((*curr != NULL) && ((*curr)->character[*str - 'a'] != NULL) &&
			TrieDeletion(&((*curr)->character[*str - 'a']), str + 1) &&
			(NULL == (*curr)))
		{
			if (!haveChildren(*curr))
			{
				free(*curr);
				(*curr) = NULL;
				return 1;
			}
			else {
				return 0;
			}
		}
	}

	// if we have reached the end of the string
	if (*str == '\0' && (NULL != (*curr)->Object))
	{
		// if current node is a leaf node and don't have any children
		if (!haveChildren(*curr))
		{
			free(*curr); // delete current node
			(*curr) = NULL;
			return 1; // delete non-leaf parent nodes
		}

		// if current node is a leaf node and have children
		else
		{
			// mark current node as non-leaf node (DON'T DELETE IT)
			(*curr)->Object = NULL;
			return 0;	   // don't delete its parent nodes
		}
	}

	return 0;
}

#if 0
// Trie Implementation in C - Insertion, Searching and Deletion
int main()
{
	struct Trie* head = getNewTrieNode();

	insert(head, "hello");
	printf("%d ", search(head, "hello"));   	// print 1

	insert(head, "helloworld");
	printf("%d ", search(head, "helloworld"));  // print 1

	printf("%d ", search(head, "helll"));   	// print 0 (Not present)

	insert(head, "hell");
	printf("%d ", search(head, "hell"));		// print 1

	insert(head, "h");
	printf("%d \n", search(head, "h")); 		// print 1 + newline

	deletion(&head, "hello");
	printf("%d ", search(head, "hello"));   	// print 0 (hello deleted)
	printf("%d ", search(head, "helloworld"));  // print 1
	printf("%d \n", search(head, "hell"));  	// print 1 + newline

	deletion(&head, "h");
	printf("%d ", search(head, "h"));   		// print 0 (h deleted)
	printf("%d ", search(head, "hell"));		// print 1
	printf("%d\n", search(head, "helloworld")); // print 1 + newline

	deletion(&head, "helloworld");
	printf("%d ", search(head, "helloworld"));  // print 0
	printf("%d ", search(head, "hell"));		// print 1

	deletion(&head, "hell");
	printf("%d\n", search(head, "hell"));   	// print 0 + newline

	if (head == NULL)
		printf("Trie empty!!\n");   			// Trie is empty now

	printf("%d ", search(head, "hell"));		// print 0

	return 0;
}
#endif // 0
