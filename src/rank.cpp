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
    for (const TDbDocument& doc : cluster.GetDocuments()) {
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
    for (const TDbDocument& doc : cluster.GetDocuments()) {
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

TWeightInfo ComputeClusterWeightPush(
    const TNewsCluster& cluster,
    const TAlexaAgencyRating& alexaAgencyRating,
    const uint64_t iterTimestamp,
    const uint64_t window,
    std::vector<double>& docWeights
) {
    std::map<std::string, double> hostScores;
    auto docs = cluster.GetDocuments();
    std::sort(docs.begin(), docs.end(),
        [](const TDbDocument& p1, const TDbDocument& p2) {
            if (p1.FetchTime != p2.FetchTime) {
                return p1.FetchTime < p2.FetchTime;
            }
            return p1.Url < p2.Url;
        });

    for (const TDbDocument& doc : cluster.GetDocuments()) {
        double agencyWeight = alexaAgencyRating.ScoreUrl(doc.Url, true);
        docWeights.push_back(agencyWeight);
    }

	double maxRank = 0.;
    int32_t clusterTime = 0;
	for (size_t i = 0; i < docs.size(); ++i) {
        const TDbDocument& startDoc = docs[i];
		int32_t startTime = startDoc.FetchTime;
		std::set<std::string> seenHosts;
		double rank = 0.;

		for (size_t j = i; j < docs.size(); ++j) {
			const TDbDocument& doc = docs[j];
			const std::string& docHost = GetHost(doc.Url);
            if (seenHosts.insert(docHost).second) {
			    double agencyWeight = alexaAgencyRating.ScoreUrl(doc.Url, true);
			    double docTimestampRemapped = static_cast<double>(static_cast<int32_t>(doc.FetchTime) - startTime) / 1800;
			    double timeMultiplier = Sigmoid(std::max(docTimestampRemapped, -15.));
			    double score = agencyWeight * timeMultiplier;
                rank += score;
            }
        }
        if (rank > maxRank) {
            maxRank = rank;
            clusterTime = startDoc.FetchTime;
        }
    }

    double timeMultiplier = 1.;

    if (clusterTime + window < iterTimestamp) {
        // ~1 for freshest ts, 0.5 for 12 hour old ts, ~0 for 24 hour old ts
        double clusterTimestampRemapped = static_cast<double>(clusterTime + static_cast<int32_t>(window) - static_cast<int32_t>(iterTimestamp)) / 3600.0 + 12.0;
        timeMultiplier = Sigmoid(clusterTimestampRemapped);
    }

    TWeightInfo info{clusterTime, maxRank, timeMultiplier, maxRank * timeMultiplier};

    return info;
}

std::vector<std::vector<TWeightedNewsCluster>> Rank(
    const TClusters& clusters,
    const TAgencyRating& agencyRating,
    const TAlexaAgencyRating& alexaAgencyRating,
    uint64_t iterTimestamp,
    uint64_t window
) {
    std::cerr << "ComputeClusterWeightPush " << iterTimestamp << " " << window << std::endl;
    std::vector<TWeightedNewsCluster> weightedClusters;
    for (const TNewsCluster& cluster : clusters) {
        tg::ECategory clusterCategory = cluster.GetCategory();
        const std::string& title = cluster.GetTitle();
        std::vector<double> docWeights;
        const TWeightInfo weight = ComputeClusterWeightPush(cluster, alexaAgencyRating, iterTimestamp, window, docWeights);
        weightedClusters.emplace_back(cluster, clusterCategory, title, weight, docWeights);
    }

    std::stable_sort(weightedClusters.begin(), weightedClusters.end(),
        [](const TWeightedNewsCluster& a, const TWeightedNewsCluster& b) {
            return a.WeightInfo.Weight > b.WeightInfo.Weight;
        }
    );

    std::vector<std::vector<TWeightedNewsCluster>> output(tg::ECategory_ARRAYSIZE);
    for (const TWeightedNewsCluster& cluster : weightedClusters) {
        assert(cluster.Category != tg::NC_UNDEFINED);
        output[static_cast<size_t>(cluster.Category)].push_back(cluster);
        output[static_cast<size_t>(tg::NC_ANY)].push_back(cluster);
    }

    return output;
}
