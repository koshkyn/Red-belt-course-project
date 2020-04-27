#pragma once

#include <iostream>
#include <istream>
#include <ostream>
#include <set>
#include <list>
#include <vector>
#include <deque>
#include <map>
#include <string_view>
#include <string>
#include <unordered_map>
#include <future>
#include <mutex>
#include <shared_mutex>

using namespace std;


struct Docid_Count {
	size_t docid;
	size_t count;
};

class InvertedIndex {
public:

  void Add(string document);
  pair<bool, const vector<Docid_Count>&> Lookup(const string_view& word) const;
  size_t GetDocsSize() {return docs.size();}
  bool DocsIsEmpty() {return docs.empty();}
  const string& GetDocument(size_t id) const {
    return docs[id];
  }

private:
  deque<string> docs;
  unordered_map<string_view, vector<Docid_Count>> index;
  
};

class SearchServer {
public:
  SearchServer() = default;
  explicit SearchServer(istream& document_input);
  void UpdateDocumentBase(istream& document_input);
  void UpdateDocumentBaseThread(istream& document_input);
  void AddQueriesSingleStream(istream& query_input, ostream& search_results_output);
  void AddQueriesStream(istream& query_input, ostream& search_results_output);
  ~SearchServer() {
	  for(auto& a : futures) {
		  a.get();
	  }
  }

private:
  InvertedIndex index;
  vector<future<void>> futures;
  mutex m;
};
