#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <memory>

#include "RadixTree.h"


RadixTree::RadixTree()
{
	rootv_ = EdgeVector::create();
}

RadixTree::~RadixTree()
{
	releaseEV(rootv_);
}

void RadixTree::releaseEV(EdgeVector *ev)
{
	for (int16_t i=0; i<ev->size_; i++) {
		Node *node = ev->nodes_[i];
		if (!node->isleaf_) {
			releaseEV((EdgeVector *)(node->data_));
		}
	}
	EdgeVector::destroy(ev);
}

void RadixTree::putToEV(const byte *key, int16_t length, const unsigned long data, struct EdgeVector *ev)
{
	int16_t ch = b2v(length, key);
	if (length > 0) {
		ch = key[0];
	}
	Node *node = ev->getAt(ch);

	// The first char never appears
	if (node == nullptr) {
		// we put the key here
		Node * nnode = Node::create(key, length, data, true);
		ev->setAt(nnode);
		return;
	}

	// Now we are sure neither key nor node->label_ is null, and they must have the same first byte
	// Conitnue to search for the longest shared prefix length between key and current node
	int len;
	for (len = 1; len < node->length_ && len < length; len++) {
		if (node->label_[len] != key[len]) {
			break;
		}
	}
	// if len == node->length_ at this point, we are sure length >= node->length_
	
	if (len == node->length_) {
		// node->label_ is a prefix of key (or the same)
		if (!node->isleaf_) {
			// if node is inner node, go to next level
			putToEV(key + len, length - len, data, (EdgeVector*)(node->data_));
			return;
		}
		else if (length == node->length_) {
			// node is leaf node and is exactly the one to put
			node->data_ = data;
			return;
		}
		// node is leaf but we have a longer key to put
	}
	
	// We need to split current node
	Node *nsub = Node::create(node->label_ + len, node->length_ - len, node->data_, node->isleaf_);
	EdgeVector *subv = EdgeVector::create();
	subv->setAt(nsub);
	Node *nsuper = Node::create(node->label_, len, uint64_t(subv), false);
	ev->setAt(nsuper);

	// Now add the new node
	Node *nnode = Node::create(key + len, length - len, data, true);
	subv->setAt(nnode);
	return;
}

void RadixTree::put(const byte *key, int16_t length, const uint64_t data)
{
	return putToEV(key, length, data, rootv_);
}

uint64_t RadixTree::getFromEV(const byte *key, int16_t length, struct EdgeVector *ev) const
{
	int16_t ch =b2v(length, key);
	struct Node *node = ev->getAt(ch);

	if (node == nullptr) {
		// No such label
		return 0UL;
	}
	else if (length < node->length_ || bcmp(key, node->label_, node->length_) != 0) {
		// If node label is not a prefix of key,
		// there is no chance to have this key
		return 0UL;
	}
	else if (length == node->length_ && node->isleaf_) {
		// If current label is 0-terminated, this is a leaf node
		return node->data_;
	}
	else {
		// goto next level
		return getFromEV(key + node->length_, length - node->length_, (EdgeVector*)(node->data_));
	}
}

uint64_t RadixTree::get(const byte *key, int16_t length) const
{
	return getFromEV(key, length, rootv_);
}

void RadixTree::removeFromEV(const byte *key, int16_t length, EdgeVector *ev)
{
	int ch = b2v(length, key);
	struct Node *node = ev->getAt(ch);

	if (node == nullptr) {
		// No such label
		return;
	}
	else if (length < node->length_ || bcmp(key, node->label_, node->length_) != 0) {
		// If node label is not a prefix of key,
		// there is no chance to have this key
		return;
	}
	else if (length == node->length_ && node->isleaf_) {
		// A leaf node
		ev->removeAt(ch);
	}
	else {
		// node is an inner node, goto next level
		EdgeVector *subv = (EdgeVector*)(node->data_);
		removeFromEV(key + node->length_, length - node->length_, subv);
		if (subv->size_ == 1) {
			// merge child
			Node *nsub = subv->nodes_[0];
			Node *nnode = Node::create(node->label_, node->length_ + nsub->length_, nsub->data_, true);
			bcopy(nsub->label_, nnode->label_ + node->length_, nsub->length_);
			EdgeVector::destroy(subv);
			ev->setAt(nnode);
		}
	}
}

void RadixTree::remove(const byte *key, int16_t length)
{
	removeFromEV(key, length, rootv_);
	return;
}

void RadixTree::dumpEV(EdgeVector * ev, char * indent) const
{
	for (int16_t ch=-1; ch<=255; ch++) {
		Node * node = ev->getAt(ch);
		if (node == nullptr) {
			continue;
		}
		
		std::unique_ptr<char> label(new char[node->length_ + 1]);
		snprintf(label.get(), node->length_ + 1, "%s", node->label_);
		if (node->isleaf_) {
			// a leaf node
			printf("%s%s: %llu\n", indent, label.get(), node->data_);
		}
		else {
			// an inner node
			printf("%s%s:\n", indent, label.get());
			strcat(indent, "    ");
			dumpEV((EdgeVector*)(node->data_), indent);
			indent[strlen(indent) - 4] = '\0';
		}
	}
	return;
}

void RadixTree::dump() const
{
	char indent[256];
	memset(indent, 0x00, 256);
	strcat(indent, "    ");
	
	printf("==============================================\n");
	printf("<root>:\n");
	dumpEV(rootv_, indent);
	printf("==============================================\n");
}

