#include "rank.h"
#include "util.h"

double ComputeClusterWeight(
    const TNewsCluster& cluster,
    const TAgencyRating& agencyRating,
    const uint64_t iterTimestamp,
    const uint64_t window,
    std::vector<double>& docWeights
) {
    std::set<std::string> seenHosts;
    double agenciesWeight = 0.0 * window;
    for (const TDocument& doc : cluster.GetDocuments()) {
        const std::string& docHost = GetHost(doc.Url);
        if (seenHosts.insert(docHost).second) {
            agenciesWeight += agencyRating.ScoreUrl(doc.Url);
            docWeights.push_back(agencyRating.ScoreUrl(doc.Url));
        } else {
            docWeights.push_back(0.);
        }
    }

    // ~1 for freshest ts, 0.5 for 12 hour old ts, ~0 for 24 hour old ts
    double clusterTimestampRemapped = static_cast<double>(cluster.GetTimestamp() - iterTimestamp) / 3600.0 + 12.0;
    double timeMultiplier = Sigmoid(clusterTimestampRemapped);

    // Pessimize only clusters with size < 5
    size_t clusterSize = cluster.GetSize();
    double smallClusterCoef = std::min(clusterSize * 0.2, 1.0);

    return agenciesWeight * timeMultiplier * smallClusterCoef;
}

double ComputeClusterWeightNew(
    const TNewsCluster& cluster,
    const TAgencyRating& agencyRating,
    const uint64_t iterTimestamp,
    const uint64_t window,
    std::vector<double>& docWeights
) {
    std::map<std::string, double> hostScores;
    double weight = 0.0 * window;
    for (const TDocument& doc : cluster.GetDocuments()) {
        const std::string& docHost = GetHost(doc.Url);
        double agencyWeight = agencyRating.ScoreUrl(doc.Url);
        // ~1 for freshest ts, 0.5 for 12 hour old ts, ~0 for 24 hour old ts
        double docTimestampRemapped = static_cast<double>(static_cast<int32_t>(doc.FetchTime) - static_cast<int32_t>(iterTimestamp)) / 3600.0 + 12.0;
        double timeMultiplier = Sigmoid(docTimestampRemapped);
        //std::cerr << doc.Url << " " << doc.FetchTime << " " << iterTimestamp << " " << docTimestampRemapped << " " << timeMultiplier << std::endl;
        double score = agencyWeight * timeMultiplier;
        if (hostScores[docHost] < score) {
            hostScores[docHost] = score;
        }
        docWeights.push_back(score);
    }

    for (auto it : hostScores) {
        weight += it.second;
    }

    // Pessimize only clusters with size < 5
    size_t clusterSize = cluster.GetSize();
    double smallClusterCoef = std::min(clusterSize * 0.2, 1.0);

    return weight * smallClusterCoef;
}

double ComputeClusterWeightPush(
    const TNewsCluster& cluster,
    const TAgencyRating& agencyRating,
    const uint64_t iterTimestamp,
    const uint64_t window,
    std::vector<double>& docWeights
) {
    std::map<std::string, double> hostScores;
    auto docs = cluster.GetDocuments();
    std::sort(docs.begin(), docs.end(),
        [](const TDocument& p1, const TDocument& p2) {
            if (p1.FetchTime != p2.FetchTime) {
                return p1.FetchTime < p2.FetchTime;
            }
            return p1.Url < p2.Url;
        });

	double maxRank = 0.;
	for (size_t i = 0; i < docs.size(); ++i) {
        const TDocument& startDoc = docs[i];
		int32_t startTime = startDoc.FetchTime;
		std::set<std::string> seenHosts;
		double rank = 0.;

		for (size_t j = i; j < docs.size(); ++j) {
			const TDocument& doc = docs[j];
			const std::string& docHost = GetHost(doc.Url);
            if (seenHosts.insert(docHost).second) {
			    double agencyWeight = agencyRating.ScoreUrl(doc.Url);
			    double docTimestampRemapped = static_cast<double>(static_cast<int32_t>(doc.FetchTime) - startTime) / 900;
			    double timeMultiplier = Sigmoid(std::max(docTimestampRemapped, -15.));
			    double score = agencyWeight * timeMultiplier;
                rank += score;
            }
        }
        if (rank > maxRank) {
            maxRank = rank;
        }
        docWeights.push_back(rank); // TODO: fix this bug
    }

    return maxRank;
}

std::vector<std::vector<TWeightedNewsCluster>> Rank(
    const TClusters& clusters,
    const TAgencyRating& agencyRating,
    uint64_t iterTimestamp,
    uint64_t window
) {
    std::cerr << "ComputeClusterWeightPush " << iterTimestamp << " " << window << std::endl;
    std::vector<TWeightedNewsCluster> weightedClusters;
    for (const TNewsCluster& cluster : clusters) {
        ENewsCategory clusterCategory = cluster.GetCategory();
        const std::string& title = cluster.GetTitle();
        std::vector<double> docWeights;
        const double weight = ComputeClusterWeightPush(cluster, agencyRating, iterTimestamp, window, docWeights);
        weightedClusters.emplace_back(cluster, clusterCategory, title, weight, docWeights);
    }

    std::stable_sort(weightedClusters.begin(), weightedClusters.end(),
        [](const TWeightedNewsCluster& a, const TWeightedNewsCluster& b) {
            return a.Weight > b.Weight;
        }
    );

    std::vector<std::vector<TWeightedNewsCluster>> output(NC_COUNT);
    for (const TWeightedNewsCluster& cluster : weightedClusters) {
        assert(cluster.Category != NC_UNDEFINED);
        output[static_cast<size_t>(cluster.Category)].push_back(cluster);
        output[static_cast<size_t>(NC_ANY)].push_back(cluster);
    }

    return output;
}
