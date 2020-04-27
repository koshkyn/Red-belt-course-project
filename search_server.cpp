#include "search_server.h"
#include "iterator_range.h"
#include "profile.h"

#include <algorithm>
#include <thread>
#include <iterator>
#include <sstream>
#include <functional>
#include <iostream>
#include <string_view>

bool operator<(const Docid_Count& lhs, const Docid_Count& rhs) { 
  		          int64_t lhs_docid = lhs.docid; // сохраняем в int, изначально docid - size_t
  		          auto lhs_hit_count = lhs.count;
  		          int64_t rhs_docid = rhs.docid; // сохраняем в int, изначально docid - size_t 
  		          auto rhs_hit_count = rhs.count;
  		          return make_pair(lhs_hit_count, -lhs_docid) > make_pair(rhs_hit_count, -rhs_docid); 
  	}



vector<string_view> SplitIntoWords(const string& line) {
	vector<string_view> result;
	string_view work_string = line;

	work_string.remove_prefix(min(work_string.find_first_not_of(" \t"), work_string.size()));

	for(size_t pos = work_string.find_first_of(" \t"); pos < work_string.size(); pos = work_string.find_first_of(" \t")) {
		result.push_back(work_string.substr(0, pos));
		work_string.remove_prefix(pos + 1);
		work_string.remove_prefix(min(work_string.find_first_not_of(" \t"), work_string.size()));
	}
	
	if(work_string.size() != 0) {
	result.push_back(work_string.substr(0, work_string.size()));
	}

	return result;
}

SearchServer::SearchServer(istream& document_input) {
  	UpdateDocumentBase(document_input);
}

void SearchServer::UpdateDocumentBase(istream& document_input) {
	// Первое заполнение базы без создания потока
	if(index.DocsIsEmpty()) {
		UpdateDocumentBaseThread(document_input);
	} else {
	this_thread::sleep_for(5ms);
	futures.push_back(async(&SearchServer::UpdateDocumentBaseThread, this, ref(document_input)));
	}
}

void SearchServer::UpdateDocumentBaseThread(istream& document_input) {
	InvertedIndex new_index;
	for (string current_document; getline(document_input, current_document); )
	{
		new_index.Add(move(current_document));
	}
	lock_guard<mutex> lg(m);
	swap(index, new_index);
}


void SearchServer::AddQueriesStream(
	istream& query_input, ostream& search_results_output
) {
	this_thread::sleep_for(5ms);
	futures.push_back(async(&SearchServer::AddQueriesSingleStream, this, ref(query_input), ref(search_results_output)));
}

void SearchServer::AddQueriesSingleStream(
  istream& query_input, ostream& search_results_output
) {
	vector<size_t> docid_count(index.GetDocsSize());
	for (string current_query; getline(query_input, current_query); ) {
	const auto words = SplitIntoWords(move(current_query));
	for (const auto& word : words) {
		lock_guard<mutex> lg(m);
		auto temp = index.Lookup(word);
		if(temp.first) {
		for(const auto& wrd : temp.second) {
			docid_count[wrd.docid] += wrd.count;
			}
		}
	}
	vector<Docid_Count> search_results;
	search_results.reserve(docid_count.size());
	for(size_t i = 0; i < docid_count.size(); ++i) {
			if(docid_count[i] != 0) {
				search_results.push_back({i, docid_count[i]});
				docid_count[i] = 0;
			}
		}
	partial_sort(search_results.begin(), search_results.begin() + min(size_t(5), search_results.size()), search_results.end());
	search_results_output << current_query << ':';
	for (auto [docid, hitcount] : Head(search_results, 5)) {
		search_results_output << " {"
		<< "docid: " << docid << ", "
		<< "hitcount: " << hitcount << '}';
	}
	search_results_output << endl;
	}
}

void InvertedIndex::Add(string document) {
  docs.push_back(move(document)); 
  map<string_view, map<size_t, size_t>> tempmap;
  const size_t docid = docs.size() - 1;
  for (const auto& word : SplitIntoWords(docs.back())) {
	  	  ++tempmap[word][docid];
	  }
  for(const auto& a : tempmap) {
	  for(const auto& b : a.second) {
		  index[a.first].push_back({b.first, b.second});
	  }
  }
}

pair<bool, const vector<Docid_Count>&> InvertedIndex::Lookup(const string_view& word) const {
  if (auto it = index.find(word); it != index.end()) {
    return {true, it->second};
  } else {
    return {false, {}};
  }
}
