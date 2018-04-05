#ifndef __RADIX_TREE_H__
#define __RADIX_TREE_H__

#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef unsigned char byte;

class RadixTree					// RadixTree
{
	static inline int16_t b2v(int16_t length, const byte b[]) {
		if (length == 0) {
			return -1;
		}
		else {
			return b[0];
		}
	}
	static const int16_t EMBEDDED_CHARS = 6;
	static const int16_t EDGES_DELTA = 4;
	struct Node
	{
		uint64_t data_; 				// data if this is a leaf node
										// or a pointer to EdgeVector
		uint16_t length_:15;			// label length
		uint16_t isleaf_:1;				// 0: inner node, 1: leaf node
		byte label_[EMBEDDED_CHARS]; 	// label

		static Node * create(const byte * label, int16_t length, uint64_t data, bool isleaf)
		{
			int16_t len = std::max(sizeof(Node) + length - EMBEDDED_CHARS, sizeof(Node));
			Node * node = (Node *)calloc(len, 1);
			node->data_ = data;
			node->length_ = length;
			node->isleaf_ = isleaf ? 1 : 0;
			bcopy(label, node->label_, length);  // copy from label to node->label_ byte by byte
			return node;
		}

		static void destroy(Node *node)
		{
			::free(node);
		}

		// If length_ == 0, return -1 as null
		// If length_ > 0, return the unsigned value of first char in label_
		// So that we defined null < '\0'
		inline int16_t firstByte()
		{
			return b2v(length_, label_);
		}
	};

	struct EdgeVector
	{
		Node ** nodes_; // pointers to Node sorted by uint8_t(Node.label_[0])
		int16_t size_;
		int16_t capacity_;

		static EdgeVector * create()
		{
			EdgeVector *ev = (EdgeVector *)malloc(sizeof(EdgeVector));
			ev->capacity_ = EDGES_DELTA;
			ev->nodes_ = (Node **)calloc(ev->capacity_ * sizeof(Node *), 1);
			ev->size_ = 0;
			return ev;
		}

		static void destroy(EdgeVector *ev) {
			for (int16_t i=0; i<ev->size_; i++) {
				Node::destroy(ev->nodes_[i]);
			}
			::free(ev->nodes_);
			::free(ev);
		}

		Node *getAt(int16_t ch) {
			// Do a binary search
			int16_t low = 0, high = size_ - 1;
			
			while (low <= high) {
				int16_t mid = (low + high) / 2;
				if (ch < nodes_[mid]->firstByte()) {
					high = mid - 1;
				}
				else if (ch > nodes_[mid]->firstByte()) {
					low = mid + 1;
				}
				else {	/* c == nodes[mid]->label_[0] */
					return nodes_[mid];
				}
			}
			// Not found
			return nullptr;
		}

		void removeAt(int16_t ch) {
			// Do a binary search
			int16_t low = 0, high = size_ - 1;

			while (low <= high) {
				int16_t mid = (low + high) / 2;
				if (ch < nodes_[mid]->firstByte()) {
					high = mid - 1;
				}
				else if (ch > nodes_[mid]->firstByte()) {
					low = mid + 1;
				}
				else {	/* c == nodes[mid]->label_[0] */
					Node * node = nodes_[mid];
					for (int16_t i=mid; i<size_ - 1; i++) {
						nodes_[i] = nodes_[i+1];
					}
					size_ --;
					if (capacity_ - size_ >= 2 * EDGES_DELTA) {
						capacity_ -= 2 * EDGES_DELTA;
						nodes_ = (Node **)realloc(nodes_, capacity_ * sizeof(Node*));
					}
					Node::destroy(node);
					return;
				}
			}
			// Not found
			return;
		}
		
		// Do *NOT* free the node to be replaced before it's replaced
		// We need the label_[0] in old node to do quick search
		void setAt(Node *node) {
			// Do a quick search
			int16_t ch = node->firstByte();
			int16_t low = 0, high = size_ - 1;
			
			while (low <= high) {
				int16_t mid = (low + high) / 2;
				if (ch < nodes_[mid]->firstByte()) {
					high = mid - 1;
				}
				else if (ch > nodes_[mid]->firstByte()) {
					low = mid + 1;
				}
				else {
					// there is a position, replace the old value
					Node::destroy(nodes_[mid]);
					nodes_[mid] = node;
					return;
				}
			}

			// Check whether we need to expand the array
			if (size_ >= capacity_) {
				capacity_ += EDGES_DELTA;
				nodes_ = (Node **)realloc(nodes_, capacity_ * sizeof(Node *));
			}
	
			// Insert the new value at current position
			int16_t i;
			for (i=size_; i>=1 && nodes_[i-1]->firstByte() > ch; i--) {
				nodes_[i] = nodes_[i-1];
			}
			nodes_[i] = node;
			size_ ++;
		}

		/** @brief
		 * Search node whose label:
		 *  - > key[0:length] if *equal == false
		 *  - >= key[0:length] if *equal == true
		 *
		 * key, *len and *equal might be changed to match the node returned
		 *
		 * @return return the node found, or nullptr if it does not exist
		 * if nullptr is returned, key and *len won't be changed
		 */
		Node * nextFrom(byte *key, int16_t *len, const int16_t max_len, bool *equal)
		{
			int16_t low = 0, high = size_ - 1;
 
			// At any time we have: [low - 1] < key < [high + 1]
			while (low <= high) {
				int16_t mid = (low + high) / 2;
				Node *node = nodes_[mid];
				int16_t minlen = std::min((*len), int16_t(node->length_));
				int ret = memcmp(key, node->label_, minlen);
				
				if (ret < 0 || (ret == 0 && (*len) < node->length_)) {
					high = mid - 1;
					if (high < low) {
						// if only previously high == low + 1 or high == low, so mid == low, and key < [mid],
						// we get mid = low and high = low - 1, thus enter into this branch
						assert(low == mid && high == mid - 1);
						// At any time we have: [low - 1] < key < [high + 1]
						// now mid == low, so we have [mid - 1] < key < [mid]
						// so [mid] is the least elemment > key
						assert(node->length_ <= max_len);
						memcpy(key, node->label_, node->length_);
						*len = node->length_;
						*equal = false;
						return node;
					}
				}
				else if (ret > 0 || (node->length_ == 0 && *len > 0)) {
					// if only previously high == low, so mid == high == low, and key > [mid]
					// we get low = mid + 1 and high = mid, thus enter into this branch
					low = mid + 1;
					if (high < low) {
						assert(high == mid && low == mid + 1);
						// At any time we have: [low - 1] < key < [high + 1]
						// now mid == high, so we have [mid] < key < [high + 1] = [mid + 1]
						// so [mid + 1] is the least element > key
						*equal = false;
						if (mid + 1 >= size_) {
							// if [mid + 1] is out of range, there is no sucn an element
							return nullptr;
						}
						else {
							node = nodes_[mid + 1];
							assert(node->length_ <= max_len);
							memcpy(key, node->label_, node->length_);
							*len = node->length_;
							return node;
						}
					}
				}
				else { // ret == 0 && (*len) >= node->length_)
					if (*equal) {
						return node;
					}
					else if (mid + 1 >= size_) {
						return nullptr;
					}
					else {
						node = nodes_[mid + 1];
						assert(node->length_ <= max_len);
						memcpy(key, node->label_, node->length_);
						*len = node->length_;
						return node;
					}
				}
			}
			// no element at all
			return nullptr;
		}
	};

	public:
		RadixTree();
		virtual ~RadixTree();
	public:
		void put(const byte *key, int16_t length, const uint64_t data);
		void put(const char *key, const uint64_t data) { return put((const byte*)key, strlen(key), data); }
		uint64_t get(const byte *key, int16_t length) const;
		uint64_t get(const char *key) const { return get((const byte*)key, strlen(key)); }
		void remove(const byte *key, int16_t length);
		void remove(const char *key) { return remove((const byte*)key, strlen(key)); }
		void dump() const;
		void next(byte *key, int16_t *len, const int16_t max_len);
		void next(char *key, const int16_t max_len)
		{
			int16_t len = strlen(key);
			next((byte*)key, &len, max_len - 1);
			key[len] = '\0';
		}
	private:
		struct EdgeVector * rootv_;
		uint64_t getFromEV(const byte *key, int16_t length, struct EdgeVector *) const;
		void putToEV(const byte *key, int16_t length, const uint64_t data, EdgeVector *ev);
		void dumpEV(EdgeVector * ev, char * indent) const;
		void removeFromEV(const byte *key, int16_t length, EdgeVector *ev);
		Node * nextInEV(byte *key, int16_t *len, const int16_t max_len, bool *equal, EdgeVector *ev);
		void releaseEV(EdgeVector *ev);
};

#endif

