/*! \file	reconstruction.cpp
 *  \brief	Definitions for recovering 3D structure from camera poses and projections.
*/

#ifdef _USE_PCL_

#include "slam/reconstruction.hpp"

void summarizeTransformation(const cv::Mat& C, char *summary) {

	cv::Mat P, R, Rv, t;
	transformationToProjection(C, P);
	decomposeTransform(C, R, t);
	Rodrigues(R, Rv);
	
	double unitsDist, degreesRot;
	unitsDist = getDistanceInUnits(t);
	degreesRot = getRotationInDegrees(R);
	
	sprintf(summary, "%f units (%f, %f, %f); %f degrees", unitsDist, t.at<double>(0,0), t.at<double>(1,0), t.at<double>(2,0), degreesRot);
	
}

int findBestCandidate(const cv::Mat *CX, const cv::Mat& K, const vector<cv::Point2f>& pts1, const vector<cv::Point2f>& pts2, cv::Mat& C) {
	
	int bestScore = 0, bestCandidate = 0;

	for (int kkk = 0; kkk < 4; kkk++) {

		int validPts = pointsInFront(CX[kkk], K, pts1, pts2);
		
		if (validPts > bestScore) {
			bestScore = validPts;
			bestCandidate = kkk;
		}
		
		//printf("%s << validPts(%d) = %d\n", __FUNCTION__, kkk, validPts);
	}
	
	CX[bestCandidate].copyTo(C);
	
	return bestScore;
}

void reverseTranslation(cv::Mat& C) {
	
	for (unsigned int iii = 0; iii < 3; iii++) {
		C.at<double>(iii,3) = -C.at<double>(iii,3);
	}
	
}



void findFeaturesForPoints(vector<featureTrack>& tracks, vector<cv::Point2f>& pts1, vector<cv::Point2f>& pts2, vector<cv::Point3d>& pts3d, int idx1, int idx2) {

	// For each 3d point, find the track that contains it, then find the projections...
	for (unsigned int jjj = 0; jjj < pts3d.size(); jjj++) {
		
		//printf("%s << jjj = %d (%f, %f, %f)\n", __FUNCTION__, jjj, pts3d.at(jjj).x, pts3d.at(jjj).y, pts3d.at(jjj).z);
		
		for (unsigned int iii = 0; iii < tracks.size(); iii++) {

			//printf("%s << iii = %d\n", __FUNCTION__, iii);
			
			// MONKEY!!
			if (tracks.at(iii).get3dLoc() == pts3d.at(jjj)) {
				
				//printf("%s << MATCH! (%d, %d)\n", __FUNCTION__, jjj, iii);
				
				for (unsigned int kkk = 0; kkk < tracks.at(iii).locations.size(); kkk++) {
					
					//printf("%s << kkk = %d (%d, %d)\n", __FUNCTION__, kkk, tracks.at(iii).locations.at(kkk).imageIndex, tracks.at(iii).locations.at(kkk).imageIndex);
			
					if (((int)tracks.at(iii).locations.at(kkk).imageIndex) == idx1) {
						
						pts1.push_back(tracks.at(iii).locations.at(kkk).featureCoord);
						
					} else if (((int)tracks.at(iii).locations.at(kkk).imageIndex) == idx2) {
						
						pts2.push_back(tracks.at(iii).locations.at(kkk).featureCoord);
						
					}
					
				}
				
			}
			
		}
		
		
	}
	
}

int countTriangulatedTracks(const vector<featureTrack>& tracks) {
	
	int triangulatedCount = 0;
	
	for (unsigned int iii = 0; iii < tracks.size(); iii++) {
		
		if (tracks.at(iii).isTriangulated) {
			triangulatedCount++;
		}
	}
	
	return triangulatedCount;
	
}

int countActiveTriangulatedTracks(vector<unsigned int>& indices, vector<featureTrack>& tracks) {
	
	int triangulatedCount = 0;
	
	for (unsigned int iii = 0; iii < indices.size(); iii++) {
		
		if (tracks.at(indices.at(iii)).isTriangulated) {
			triangulatedCount++;
		}
		
	}
	
	return triangulatedCount;
	
}

void getIndicesForTriangulation(vector<unsigned int>& dst, vector<unsigned int>& src, vector<unsigned int>& already_triangulated) {
	
	dst.clear();
	
	for (unsigned int iii = 0; iii < src.size(); iii++) {
		
		bool triangulated = false;
		
		for (unsigned int jjj = 0; jjj < already_triangulated.size(); jjj++) {
			
			if (already_triangulated.at(jjj) == src.at(iii)) {
				triangulated = true;
				continue;
			}

			
		}
		
		if (!triangulated) {
			
			dst.push_back(src.at(iii));
			
		}

		
	}
	
}

void reconstructSubsequence(vector<featureTrack>& tracks, vector<cv::Point3d>& ptCloud, int idx1, int idx2) {
	
}

void removeShortTracks(vector<featureTrack>& tracks, int idx1, int idx2) {
	
	int minLength = (idx2 - idx1) / 2;
	
	for (unsigned int iii = 0; iii < tracks.size(); iii++) {
		int trackLength = tracks.at(iii).locations.size();
		if (trackLength < minLength) {
			int final_image = tracks.at(iii).locations.at(trackLength-1).imageIndex;
			
			if ((final_image >= idx1) && (final_image < idx2)) {
				tracks.erase(tracks.begin() + iii);
				iii--;
			}
			
		}
	}
	
}

void updateTriangulatedPoints(vector<featureTrack>& tracks, vector<unsigned int>& indices, vector<cv::Point3d>& cloud) {
	
	if (cloud.size() != indices.size()) {
		printf("%s << ERROR! Cloud size and index list size don't match (%d vd %d)\n", __FUNCTION__, ((int)cloud.size()), ((int)indices.size()));
	}
	
	for (unsigned int iii = 0; iii < indices.size(); iii++) {
		
		if (tracks.at(indices.at(iii)).isTriangulated) {
			//printf("%s << (%d) is triangulated; moving from (%f, %f, %f) to (%f, %f, %f)\n", __FUNCTION__, iii, tracks.at(indices.at(iii)).get3dLoc().x, tracks.at(indices.at(iii)).get3dLoc().y, tracks.at(indices.at(iii)).get3dLoc().z, cloud.at(iii).x, cloud.at(iii).y, cloud.at(iii).z);
		}
		
		tracks.at(indices.at(iii)).set3dLoc(cloud.at(iii));
		
		
		
	}
	
}

void reduceActiveToTriangulated(vector<featureTrack>& tracks, vector<unsigned int>& indices, vector<unsigned int>& untriangulated) {
	
	for (unsigned int iii = 0; iii < indices.size(); iii++) {
		
		if (!tracks.at(indices.at(iii)).isTriangulated) {
			untriangulated.push_back(indices.at(iii));
			indices.erase(indices.begin() + iii);
			iii--;
		}
		
	}
	
}

void filterToActivePoints(vector<featureTrack>& tracks, vector<cv::Point2f>& pts1, vector<cv::Point2f>& pts2, vector<unsigned int>& indices, int idx1, int idx2) {

	pts1.clear();
	pts2.clear();
	
	for (unsigned int iii = 0; iii < indices.size(); iii++) {
		
		for (unsigned int jjj = 0; jjj < tracks.at(indices.at(iii)).locations.size()-1; jjj++) {
			
			if (((int)tracks.at(indices.at(iii)).locations.at(jjj).imageIndex) == idx1) {
				
				pts1.push_back(tracks.at(indices.at(iii)).locations.at(jjj).featureCoord);
				
				for (unsigned int kkk = jjj+1; kkk < tracks.at(indices.at(iii)).locations.size(); kkk++) {
					
					if (((int)tracks.at(indices.at(iii)).locations.at(kkk).imageIndex) == idx2) {
						
						pts2.push_back(tracks.at(indices.at(iii)).locations.at(kkk).featureCoord);
						
						continue;
					}
					
				}
				
				continue;
				
			}
			
		}

	}
	
	
}

void getActive3dPoints(vector<featureTrack>& tracks, vector<unsigned int>& indices, vector<cv::Point3d>& cloud) {
	
	cloud.clear();
	
	for (unsigned int iii = 0; iii < indices.size(); iii++) {
		cloud.push_back(tracks.at(indices.at(iii)).get3dLoc());
	}
	
}

void filterToCompleteTracks(vector<unsigned int>& dst, vector<unsigned int>& src, vector<featureTrack>& tracks, int idx1, int idx2) {
	
	dst.clear();
	
	for (unsigned int iii = 0; iii < src.size(); iii++) {
		
		if (int(tracks.at(src.at(iii)).locations.size()) >= (idx2 - idx1)) {
			
			for (unsigned int jjj = 0; jjj < tracks.at(src.at(iii)).locations.size()-(idx2-idx1); jjj++) {
				
				//printf("%s << DEBUG [%d][%d]\n", __FUNCTION__, iii, jjj);
			
				if (((int)tracks.at(src.at(iii)).locations.at(jjj).imageIndex) == idx1) {
					
					//printf("%s << DEBUG [%d][%d] X\n", __FUNCTION__, iii, jjj);
					
					for (unsigned int kkk = jjj+1; kkk < tracks.at(src.at(iii)).locations.size(); kkk++) {
						
						//printf("%s << DEBUG [%d][%d][%d]\n", __FUNCTION__, iii, jjj, kkk);
						
						if (((int)tracks.at(src.at(iii)).locations.at(kkk).imageIndex) == idx2) {
							
							//printf("%s << DEBUG [%d][%d][%d] X\n", __FUNCTION__, iii, jjj, kkk);
							
							dst.push_back(src.at(iii));
							
						}
						
					}
					
				}
				
				
			}
			
		}
		
	}
	
}

void getActiveTracks(vector<unsigned int>& indices, vector<featureTrack>& tracks, int idx1, int idx2) {
	
	//printf("%s << Entered.\n", __FUNCTION__);
	
	// Should only consider a track active if it contains at least two projections in the subsequence
	
	indices.clear();
	
	for (unsigned int iii = 0; iii < tracks.size(); iii++) {
		
		//printf("%s << DEBUG [%d]\n", __FUNCTION__, iii);
		
		if (tracks.at(iii).locations.size() < 2) {
			continue;
		}
		
		for (unsigned int jjj = 0; jjj < tracks.at(iii).locations.size()-1; jjj++) {
			
			//printf("%s << DEBUG [%d][%d]\n", __FUNCTION__, iii, jjj);
		
			if ((((int)tracks.at(iii).locations.at(jjj).imageIndex) >= idx1) && (((int)tracks.at(iii).locations.at(jjj).imageIndex) <= idx2)) {
				
				for (unsigned int kkk = jjj+1; kkk < tracks.at(iii).locations.size(); kkk++) {
					
					if ((((int)tracks.at(iii).locations.at(kkk).imageIndex) >= idx1) && (((int)tracks.at(iii).locations.at(kkk).imageIndex) <= idx2)) {
						indices.push_back(iii);
						break;
					}
					
				}
				
				break;
			}
			
			
		}
		
		
		
		
		
	}
	
	//printf("%s << Exiting.\n", __FUNCTION__);
	
}

bool estimatePoseFromKnownPoints(cv::Mat& dst, cameraParameters camData, vector<featureTrack>& tracks, unsigned int index, const cv::Mat& guide, unsigned int minAppearances, unsigned int iterCount, double maxReprojErr, double inliersPercentage, double *reprojError, double *pnpInlierProp, bool debug) {
	
	vector<cv::Point3f> points_3d;
	vector<cv::Point2f> points_2d;
	
	unsigned int triangulatedCount = 0;
	*reprojError = 0.0;
	*pnpInlierProp = 0.0;
	
	if (debug) { printf("%s << minAppearances = (%d)\n", __FUNCTION__, minAppearances); }
	
	for (unsigned int iii = 0; iii < tracks.size(); iii++) {
		
		//printf("%s << DEBUG [%d]\n", __FUNCTION__, iii);
		
		if (!tracks.at(iii).isTriangulated) {
			continue;
		}
		
		triangulatedCount++;
		
		if (tracks.at(iii).locations.size() < minAppearances) {
			printf("%s << Error! size() = (%lu) < (%d)\n", __FUNCTION__, tracks.at(iii).locations.size(), minAppearances);
			continue;
		}
		
		bool pointAdded = false;
		
		for (unsigned int jjj = 0; jjj < tracks.at(iii).locations.size(); jjj++) {
			
			//printf("%s << DEBUG [%d][%d]\n", __FUNCTION__, iii, jjj);
		
			if (tracks.at(iii).locations.at(jjj).imageIndex == index) {
				
				points_2d.push_back(tracks.at(iii).locations.at(jjj).featureCoord);
				cv::Point3f tmp_pt = cv::Point3f(((float) tracks.at(iii).get3dLoc().x), ((float) tracks.at(iii).get3dLoc().y), ((float) tracks.at(iii).get3dLoc().z));
				points_3d.push_back(tmp_pt);
				pointAdded = true;
				break;
			}
		}
		
		/*
		if (!pointAdded) {
			printf("%s << Error! Pt (%d) was not found in current view (%d)\n", __FUNCTION__, iii, index);
			for (unsigned int jjj = 0; jjj < tracks.at(iii).locations.size(); jjj++) {
				printf("%s << Index (%d)...\n", __FUNCTION__, tracks.at(iii).locations.at(jjj).imageIndex);
			}
			
		}
		*/
		
	}
	
	unsigned int utilizedPointsCount = points_3d.size();
	
	cv::Mat Rvec, R, t;
	bool guided = false;
	
	cv::Mat guide_copy;
	
	guide_copy = guide.inv();
	
	if (guide_copy.rows > 0) {
		decomposeTransform(guide_copy, R, t);
		Rodrigues(R, Rvec);
		guided = true;
	}
	
	//cv::Mat inlierPts;
	vector<int> inlierPts;
	
	if (points_2d.size() < 8) {
		if (debug) { printf("%s << PnP Failed: Insufficient pts: (%d) < (%d) : [%d, %d, %d]\n", __FUNCTION__, ((int)points_2d.size()), 8, utilizedPointsCount, triangulatedCount, ((int)tracks.size())); }
		guide.copyTo(dst);
		return false;
	}
	
	#ifdef _OPENCV_VERSION_3_PLUS_
	solvePnPRansac(points_3d, points_2d, camData.K, camData.blankCoeffs, Rvec, t, guided, iterCount, float(maxReprojErr), ((unsigned int) (((double) points_2d.size()) * inliersPercentage)), inlierPts, cv::EPNP); // ITERATIVE, P3P
	#else
	solvePnPRansac(points_3d, points_2d, camData.K, camData.blankCoeffs, Rvec, t, guided, iterCount, float(maxReprojErr), ((unsigned int) (((double) points_2d.size()) * inliersPercentage)), inlierPts, CV_EPNP); // ITERATIVE, CV_P3P
	#endif
	
	
	
	//if ( ((unsigned int) inlierPts.size()) < ((unsigned int) (((double) points_2d.size()) * inliersPercentage)) ) {
	if ( inlierPts.size() < 8 ) {
		if (debug) { printf("%s << PnP Failed:  Insufficient inliers: (%lu) / (%lu) : [%d, %d, %d]\n", __FUNCTION__, inlierPts.size(), points_2d.size(), utilizedPointsCount, triangulatedCount, ((int)tracks.size())); }
		guide.copyTo(dst);
		return false;
	}
	
	if (debug) { printf("%s << PnP Succeeded:  Inliers: (%d) / (%lu) : [%d, %d, %d]\n", __FUNCTION__, (int)inlierPts.size(), points_2d.size(), utilizedPointsCount, triangulatedCount, ((int)tracks.size())); }
	
	*pnpInlierProp = double(inlierPts.size()) / double(points_2d.size());
	
	Rodrigues(Rvec, R);
	cv::Mat P;
	findP1Matrix(P, R, t);			
	projectionToTransformation(P, dst);
	dst = dst.inv();
	
	// Obtain some kind of error
	
	//printf("%s << points3d.size() = (%d); guided.size() = (%d)\n", __FUNCTION__, points_3d.size(), guided.size());
	//double reprojError = 0.0;
	
	//printf("%s << debug (%d)\n", __FUNCTION__, 0);
	
	std::vector<cv::Point3f> validPts;
	for (unsigned int iii = 0; iii < inlierPts.size(); iii++) {
		validPts.push_back(points_3d.at(inlierPts.at(iii)));
	}
	
	//printf("%s << debug (%d)\n", __FUNCTION__, 1);
	std::vector<cv::Point2f> point_2f;
	projectPoints(validPts, Rvec, t, camData.K, camData.blankCoeffs, point_2f);
	
	//printf("%s << debug (%d)\n", __FUNCTION__, 2);
	
	for (unsigned int iii = 0; iii < inlierPts.size(); iii++) {
		//printf("%s << points_2d.at(%d) = (%f, %f), point_2f.at(%d) = (%f, %f)\n", __FUNCTION__, iii, points_2d.at(inlierPts.at(iii)).x, points_2d.at(inlierPts.at(iii)).y, iii, point_2f.at(iii).x, point_2f.at(iii).y);
		*reprojError += distBetweenPts2f(points_2d.at(inlierPts.at(iii)), point_2f.at(iii));
	
	}
	
	if (inlierPts.size() > 0) {
		*reprojError /= double(inlierPts.size());
	} else {
		*reprojError = -1.0;
	}
	
	//cout << "guide = " << guide << endl;
	//cout << "dst = " << dst << endl;
	
	return true;
	
}



void getCorrespondingPoints(vector<featureTrack>& tracks, const vector<cv::Point2f>& pts1, vector<cv::Point2f>& pts2, int idx0, int idx1) {
	
	printf("%s << Entered X.\n", __FUNCTION__);
	
	// For each track
	for (unsigned int iii = 0; iii < tracks.size(); iii++) {
		
		if (tracks.at(iii).locations.size() < 2) {
			continue;
		}
		//printf("%s << Debug (%d)\n", __FUNCTION__, iii);
		// For each projection in the track
		for (unsigned int kkk = 0; kkk < tracks.at(iii).locations.size()-1; kkk++) {
		
			//printf("%s << Debug (%d)(%d)\n", __FUNCTION__, iii, kkk);
			
			// If the camera of the projection matches the aim (idx0)
			if (((int)tracks.at(iii).locations.at(kkk).imageIndex) == idx0) {
				// Found a feature that exists in the first image
				
				//printf("%s << Debug (%d)(%d) 2\n", __FUNCTION__, iii, kkk);
				
				// For each of the first set of points
				for (unsigned int jjj = 0; jjj < pts1.size(); jjj++) {
				
					//printf("%s << Debug (%d)(%d)(%d)\n", __FUNCTION__, iii, kkk, jjj);
				
					// If the projection location matches the first point at this point 
					if ((tracks.at(iii).locations.at(kkk).featureCoord.x == pts1.at(jjj).x) && (tracks.at(iii).locations.at(kkk).featureCoord.y == pts1.at(jjj).y)) {
						
						//printf("%s << Debug (%d)(%d)(%d) 2\n", __FUNCTION__, iii, kkk, jjj);
						
						// For each of the later projections on that track
						for (unsigned int ppp = kkk; ppp < tracks.at(iii).locations.size(); ppp++) {
							
							//printf("%s << Debug (%d)(%d)(%d)(%d)\n", __FUNCTION__, iii, kkk, jjj, ppp);
							
							// If this later projection matches the second aimed index
							if (((int)tracks.at(iii).locations.at(ppp).imageIndex) == idx1) {
								
								//printf("%s << Debug (%d)(%d)(%d)(%d) 2\n", __FUNCTION__, iii, kkk, jjj, ppp);
								
								// If the first projec....?
								if (jjj != pts2.size()) {
									printf("%s << ERROR! Vectors are not syncing (jjj = %d), pts2.size() = %d\n", __FUNCTION__, jjj, ((int)pts2.size()));
								} else {
									pts2.push_back(tracks.at(iii).locations.at(ppp).featureCoord);
								}

								
							}
	
							
						}
						
						break;
					}
					
				}
				
				
				break;
			}
			
		}
	}
		
	printf("%s << Exiting.\n", __FUNCTION__);
	
}

void getTriangulatedFullSpanPoints(vector<featureTrack>& tracks, vector<cv::Point2f>& pts1, vector<cv::Point2f>& pts2, int idx1, int idx2, vector<cv::Point3f>& points3) {
	
	pts1.clear();
	pts2.clear();
	points3.clear();
	
	for (unsigned int iii = 0; iii < tracks.size(); iii++) {
		
		for (unsigned int kkk = 0; kkk < tracks.at(iii).locations.size()-1; kkk++) {
			
			// If the track extends between the two images of interest
			if (((int)tracks.at(iii).locations.at(kkk).imageIndex) == idx1) {
				
				for (unsigned int jjj = kkk+1; jjj < tracks.at(iii).locations.size(); jjj++) {
					
					if (((int)tracks.at(iii).locations.at(jjj).imageIndex) == idx2) {
						
						if (tracks.at(iii).isTriangulated) {
							
							pts1.push_back(tracks.at(iii).locations.at(kkk).featureCoord);
							pts2.push_back(tracks.at(iii).locations.at(jjj).featureCoord);
							
							points3.push_back(cv::Point3f(((float) tracks.at(iii).get3dLoc().x), ((float) tracks.at(iii).get3dLoc().y), ((float) tracks.at(iii).get3dLoc().z)));
							
							break;
						}
						
						
						
					}
				}
				
			}
		}
	}
	
}

bool findClusterMean(const vector<cv::Point3d>& pts, cv::Point3d& pt3d, int mode, int minEstimates, double maxStandardDev) {
	
	cv::Point3d mean3d = cv::Point3d(0.0, 0.0, 0.0);
	cv::Point3d stddev3d = cv::Point3d(0.0, 0.0, 0.0);
	
	vector<cv::Point3d> estimatedLocations;
	estimatedLocations.insert(estimatedLocations.end(), pts.begin(), pts.end());
	vector<int> clusterCount;
	
	double outlierLimit = 3.0;
	
	if (mode == CLUSTER_MEAN_MODE) {
		
		
		for (unsigned int iii = 0; iii < estimatedLocations.size(); iii++) {
			clusterCount.push_back(1);
			//printf("%s << pt(%d) = (%f, %f, %f)\n", __FUNCTION__, iii, estimatedLocations.at(iii).x, estimatedLocations.at(iii).y, estimatedLocations.at(iii).z);
		}
		
		for (unsigned int iii = 0; iii < estimatedLocations.size()-1; iii++) {
			
			cv::Point3d basePt = estimatedLocations.at(iii);
			
			
			
			for (unsigned int jjj = iii+1; jjj < estimatedLocations.size(); jjj++) {
				
				double separation = pow(pow(basePt.x - estimatedLocations.at(jjj).x, 2.0) + pow(basePt.y - estimatedLocations.at(jjj).y, 2.0) + pow(basePt.z - estimatedLocations.at(jjj).z, 2.0), 0.5);
				
				if ( (separation < maxStandardDev) || (maxStandardDev == 0.0) ) {
					
					estimatedLocations.at(iii) *= double(clusterCount.at(iii));
					clusterCount.at(iii)++;
					estimatedLocations.at(iii) += estimatedLocations.at(jjj);
					estimatedLocations.at(iii).x /= double(clusterCount.at(iii));
					estimatedLocations.at(iii).y /= double(clusterCount.at(iii));
					estimatedLocations.at(iii).z /= double(clusterCount.at(iii));
					
					estimatedLocations.erase(estimatedLocations.begin() + jjj);
					clusterCount.erase(clusterCount.begin() + jjj);
					jjj--;
					
				}
				
			}
		}
		
		int maxClusterSize = 0, maxClusterIndex = -1;
		
		for (unsigned int iii = 0; iii < estimatedLocations.size(); iii++) {

			if (clusterCount.at(iii) >= maxClusterSize) {
				maxClusterSize = clusterCount.at(iii);
				maxClusterIndex = iii;
			}

			//printf("%s << cluster(%d) = (%f, %f, %f) [%d]\n", __FUNCTION__, iii, estimatedLocations.at(iii).x, estimatedLocations.at(iii).y, estimatedLocations.at(iii).z, clusterCount.at(iii));
		}
		
		if (maxClusterSize >= minEstimates) {
			pt3d = estimatedLocations.at(maxClusterIndex);
		} else {
			return false;
		}
		
	} else {
		//printf("%s << A; estimatedLocations.size() = (%d)\n", __FUNCTION__, estimatedLocations.size());
		
		for (unsigned int qqq = 0; qqq < estimatedLocations.size(); qqq++) {
			
			//printf("%s << pt(%d) = (%f, %f, %f)\n", __FUNCTION__, qqq, estimatedLocations.at(qqq).x, estimatedLocations.at(qqq).y, estimatedLocations.at(qqq).z); /* , separationsVector.at(qqq) */
			
			mean3d.x += (estimatedLocations.at(qqq).x / ((double) estimatedLocations.size()));
			mean3d.y += (estimatedLocations.at(qqq).y / ((double) estimatedLocations.size()));
			mean3d.z += (estimatedLocations.at(qqq).z / ((double) estimatedLocations.size()));
			
		}
		
		//printf("%s << mean = (%f, %f, %f)\n", __FUNCTION__, mean3d.x, mean3d.y, mean3d.z);
		
		//printf("%s << mean point = (%f, %f, %f)\n", __FUNCTION__, mean3d.x, mean3d.y, mean3d.z);
		
		// Calculate initial standard deviation
		for (unsigned int qqq = 0; qqq < estimatedLocations.size(); qqq++) {
			
			stddev3d.x += (pow((estimatedLocations.at(qqq).x - mean3d.x), 2.0) / ((double) estimatedLocations.size()));
			stddev3d.y += (pow((estimatedLocations.at(qqq).y - mean3d.y), 2.0) / ((double) estimatedLocations.size()));
			stddev3d.z += (pow((estimatedLocations.at(qqq).z - mean3d.z), 2.0) / ((double) estimatedLocations.size()));
			
		}
		
		stddev3d.x = pow(stddev3d.x, 0.5);
		stddev3d.y = pow(stddev3d.y, 0.5);
		stddev3d.z = pow(stddev3d.z, 0.5);
		
		//printf("%s << stddev3d = (%f, %f, %f)\n", __FUNCTION__, stddev3d.x, stddev3d.y, stddev3d.z);
		
		//printf("%s << Point triangulated from (%d) view pairs: (%f, %f, %f) / (%f, %f, %f)\n", __FUNCTION__, estimatedLocations.size(), mean3d.x, mean3d.y, mean3d.z, stddev3d.x, stddev3d.y, stddev3d.z);
		
		// Reject projections that are more than X standard deviations away
		for (int qqq = estimatedLocations.size()-1; qqq >= 0; qqq--) {
			
			double abs_diff_x = abs(estimatedLocations.at(qqq).x - mean3d.x);
			double abs_diff_y = abs(estimatedLocations.at(qqq).y - mean3d.y); 
			double abs_diff_z = abs(estimatedLocations.at(qqq).z - mean3d.z); 
			
			if ((abs_diff_x > outlierLimit*stddev3d.x) || (abs_diff_y > outlierLimit*stddev3d.y) || (abs_diff_z > outlierLimit*stddev3d.z)) {
				estimatedLocations.erase(estimatedLocations.begin() + qqq);
			}

		}
		
		//printf("%s << B: estimatedLocations.size() = (%d)\n", __FUNCTION__, estimatedLocations.size());

		// Recalculate the standard deviation
		stddev3d.x = 0.0;
		stddev3d.y = 0.0;
		stddev3d.z = 0.0;
		
		for (unsigned int qqq = 0; qqq < estimatedLocations.size(); qqq++) {
			
			stddev3d.x += (pow((estimatedLocations.at(qqq).x - mean3d.x), 2.0) / ((double) estimatedLocations.size()));
			stddev3d.y += (pow((estimatedLocations.at(qqq).y - mean3d.y), 2.0) / ((double) estimatedLocations.size()));
			stddev3d.z += (pow((estimatedLocations.at(qqq).z - mean3d.z), 2.0) / ((double) estimatedLocations.size()));
			
		}
		
		stddev3d.x = pow(stddev3d.x, 0.5);
		stddev3d.y = pow(stddev3d.y, 0.5);
		stddev3d.z = pow(stddev3d.z, 0.5);
		
		//printf("%s << stddev3d = (%f, %f, %f) [%d]\n", __FUNCTION__, stddev3d.x, stddev3d.y, stddev3d.z, minEstimates);
		
		// Reject track if the standard deviation is still too high, or not enough rays remain
		if ( ((stddev3d.x > maxStandardDev) || (stddev3d.y > maxStandardDev) || (stddev3d.z > maxStandardDev) || (((int)estimatedLocations.size()) < minEstimates)) && (maxStandardDev != 0.0) ) { 
			return false;
		}
		
		//printf("%s << C: estimatedLocations.size() = (%d)\n", __FUNCTION__, estimatedLocations.size());
		
		// Final calculation of average point location
		pt3d.x = 0.0;
		pt3d.y = 0.0;
		pt3d.z = 0.0;
		
		for (unsigned int qqq = 0; qqq < estimatedLocations.size(); qqq++) {
				
			pt3d.x += (estimatedLocations.at(qqq).x / ((double) estimatedLocations.size()));
			pt3d.y += (estimatedLocations.at(qqq).y / ((double) estimatedLocations.size()));
			pt3d.z += (estimatedLocations.at(qqq).z / ((double) estimatedLocations.size()));
			
		}
	}
	
	return true;
}

void obtainAppropriateBaseTransformation(cv::Mat& C0, vector<featureTrack>& tracks) {
	
	cv::Point3d centroid, deviations;
	
	int numTriangulatedPts = 0;
	
	for (unsigned int iii = 0; iii < tracks.size(); iii++) {
		if (tracks.at(iii).isTriangulated) {
			
			numTriangulatedPts += 1;
			
		}
	}
	
	if (numTriangulatedPts == 0) {
		C0 = cv::Mat::eye(4, 4, CV_64FC1);
	} else {
		// Find centroid and standard deviations of points
		findCentroidAndSpread(tracks, centroid, deviations);
		
		// Create transformation matrix with a centroid that is distance
		cv::Mat t(4, 1, CV_64FC1), R;
		
		R = cv::Mat::eye(3, 3, CV_64FC1);
		
		t.at<double>(0,0) = centroid.x + 3.0 * deviations.x;
		t.at<double>(0,0) = centroid.y + 3.0 * deviations.y;
		t.at<double>(0,0) = centroid.z + 3.0 * deviations.z;
		t.at<double>(3,0) = 1.0;

		composeTransform(R, t, C0);
	}
}

void findCentroidAndSpread(vector<featureTrack>& tracks, cv::Point3d& centroid, cv::Point3d& deviations) {
	
	centroid = cv::Point3d(0.0, 0.0, 0.0);
	deviations = cv::Point3d(0.0, 0.0, 0.0);
	
	double numPoints = 0.00;
	
	for (unsigned int iii = 0; iii < tracks.size(); iii++) {
		if (tracks.at(iii).isTriangulated) {
			
			numPoints += 1.00;
			
		}
	}
	
	if (numPoints == 0.00) {
		return;
	}
	
	for (unsigned int iii = 0; iii < tracks.size(); iii++) {
		
		if (tracks.at(iii).isTriangulated) {
			
			cv::Point3d tmpPt;
			tmpPt = tracks.at(iii).get3dLoc();
			centroid.x += (tmpPt.x / numPoints);
			centroid.y += (tmpPt.y / numPoints);
			centroid.z += (tmpPt.z / numPoints);
			
		}
		
	}
	
	for (unsigned int iii = 0; iii < tracks.size(); iii++) {
		
		if (tracks.at(iii).isTriangulated) {
			cv::Point3d tmpPt;
			tmpPt = tracks.at(iii).get3dLoc();
			deviations.x += (pow((tmpPt.x - centroid.x), 2.0) / numPoints);
			deviations.y += (pow((tmpPt.y - centroid.y), 2.0) / numPoints);
			deviations.z += (pow((tmpPt.z - centroid.z), 2.0) / numPoints);
		}
		
	}
	
	deviations.x = pow(deviations.x, 0.5);
	deviations.y = pow(deviations.y, 0.5);
	deviations.z = pow(deviations.z, 0.5);
}




void findFourTransformations(cv::Mat *C, const cv::Mat& E, const cv::Mat& K, const vector<cv::Point2f>& pts1, const vector<cv::Point2f>& pts2) {
	
	
	cv::Mat I, negI, t33, W, Winv, Z, Kinv, zeroTrans, R, t, Rvec;
	
	Kinv = K.inv();
	zeroTrans = cv::Mat::zeros(3, 1, CV_64FC1);
	I = cv::Mat::eye(3, 3, CV_64FC1);
	negI = -I;
	
	getWandZ(W, Winv, Z);
	
	//cout << __FUNCTION__ << " << W = " << W << endl;
	//cout << __FUNCTION__ << " << Winv = " << Winv << endl;
	
	//R = svd.u * W * svd.vt;
	//t = svd.u.col(2);
	
	//cout << __FUNCTION__ << " << R = " << R << endl;
	
	//Rodrigues(R, Rvec);
	
	//cout << __FUNCTION__ << " << Rvec = " << Rvec << endl;
	
	//cout << __FUNCTION__ << " << t = " << t << endl;
	
	cv::Mat c;
	//compileTransform(c, R, t);
	
	//cout << __FUNCTION__ << " << c = " << c << endl;
	
	cv::Mat u_transposePos, u_transposeNeg;
	cv::SVD svdPos, svdNeg;
	
	svdPos = cv::SVD(E);
	svdNeg = cv::SVD(-E);
		
	transpose(svdPos.u, u_transposePos);
	transpose(svdNeg.u, u_transposeNeg);
	
	double detVal;
	
	for (unsigned int zzz = 0; zzz < 4; zzz++) {
		
		//printf("%s << Loop [%d]\n", __FUNCTION__, zzz);
		
		switch (zzz) {
			case 0:
				//t33 = v * W * SIGMA * vt;
				t33 = svdPos.u * Z * u_transposePos;
				R = svdPos.u * Winv * svdPos.vt;
				detVal = determinant(R);
				if (detVal < 0) {
					t33 = svdNeg.u * Z * u_transposeNeg;
					R = svdNeg.u * Winv * svdNeg.vt;
				}
				break;
			case 1:
				//t33 = v * W_inv * SIGMA * vt;
				t33 = negI * svdPos.u * Z * u_transposePos;
				R = svdPos.u * Winv * svdPos.vt;
				detVal = determinant(R);
				if (detVal < 0) {
					t33 = negI * svdNeg.u * Z * u_transposeNeg;
					R = svdNeg.u * Winv * svdNeg.vt;
				}
				break;
			case 2:
				//t33 = v * W * SIGMA * vt;
				t33 = svdPos.u * Z * u_transposePos;
				R = svdPos.u * W * svdPos.vt;
				detVal = determinant(R);
				if (detVal < 0) {
					t33 = svdNeg.u * Z * u_transposeNeg;
					R = svdNeg.u * W * svdNeg.vt;
				}
				break;
			case 3:
				//t33 = v * W_inv * SIGMA * vt;
				t33 = negI * svdPos.u * Z * u_transposePos;
				R = svdPos.u * W * svdPos.vt;
				detVal = determinant(R);
				if (detVal < 0) {
					t33 = negI * svdNeg.u * Z * u_transposeNeg;
					R = svdNeg.u * W * svdNeg.vt;
				}
				break;
			default:
				break;
		}
		
		// Converting 3x3 t mat to vector
		t = cv::Mat::zeros(3, 1, CV_64F);
		t.at<double>(0,0) = -t33.at<double>(1,2); // originally had -ve
		t.at<double>(1,0) = t33.at<double>(0,2); // originall had a +ve
		t.at<double>(2,0) = -t33.at<double>(0,1); // originally had -ve
		
		//cout << __FUNCTION__ << " << t[" << zzz << "] = " << t << endl;
		
		if (abs(1.0 - detVal) > 0.01) {
			//printf("%s << Solution [%d] is invalid - det(R) = %f\n", __FUNCTION__, zzz, detVal);
		} else {
			//printf("%s << Solution [%d] det(R) = %f\n", __FUNCTION__, zzz, detVal);
		}
		
		//compileTransform(c, R, -R*t);
		//compileTransform(c, R, t);
		composeTransform(R, t, c);
		
		//char printSummary[256];
		//summarizeTransformation(c, printSummary);
		//printf("%s << Original c[%d] = %s\n", __FUNCTION__, zzz, printSummary);
		
		//c = c.inv();
		
		c.copyTo(C[zzz]);
		
		//cout << __FUNCTION__ << " << Czzz = " << C[zzz] << endl;
		
	}

}

bool pointIsInFront(const cv::Mat& C, const cv::Point3d& pt) {
	
	cv::Point3d p1;
	transfer3dPoint(pt, p1, C);

	if (p1.z > 0) return true;
	return false;
}

int pointsInFront(const cv::Mat& C1, const cv::Mat& C2, const vector<cv::Point3d>& pts) {
	int retVal = 0;
	
	vector<cv::Point3d> pts1, pts2;
	
	cv::Mat R1, t1, R2, t2;
	decomposeTransform(C1, R1, t1);
	decomposeTransform(C2, R2, t2);
	
	double rot1, rot2;
	rot1 = getRotationInDegrees(R1);
	rot2 = getRotationInDegrees(R2);
	
	printf("%s << Two rotations = (%f, %f)\n", __FUNCTION__, rot1, rot2);
	
	// maybe something to do with intrinsics matrix....
	
	// want to transfer the 3D point locations into the co-ordinate system of each camera
	transfer3DPoints(pts, pts1, C1);
	transfer3DPoints(pts, pts2, C2);
	
	for (unsigned int iii = 0; iii < pts.size(); iii++) {
		
		//printf("%s << [%d](%f, %f, %f)-(%f, %f, %f)\n", __FUNCTION__, iii, pts1.at(iii).x, pts1.at(iii).y, pts1.at(iii).z, pts2.at(iii).x, pts2.at(iii).y, pts2.at(iii).z);
		
		if ((pts1.at(iii).z > 0) && (pts2.at(iii).z > 0)) {
			
			if (retVal < 5) {
				printf("%s << Point location is (%f, %f, %f)\n", __FUNCTION__, pts.at(iii).x, pts.at(iii).y, pts.at(iii).z);
				printf("%s << Rel #1 is (%f, %f, %f)\n", __FUNCTION__, pts1.at(iii).x, pts1.at(iii).y, pts1.at(iii).z);
				printf("%s << Rel #2 is (%f, %f, %f)\n", __FUNCTION__, pts2.at(iii).x, pts2.at(iii).y, pts2.at(iii).z);
			}
			
			retVal++;
		}
	}
	
	return retVal;
}

int pointsInFront(const cv::Mat& C1, const cv::Mat& K, const vector<cv::Point2f>& pts1, const vector<cv::Point2f>& pts2) {
	
	int retVal = 0;
	
	cv::Mat Kinv;
	Kinv = K.inv();
	
	// Representing initial camera relative to itself
	cv::Mat P0, C0;
	initializeP0(P0);
	projectionToTransformation(P0, C0);
	
	// Representing second camera relative to first camera
	cv::Mat P1;
	transformationToProjection(C1, P1);
	

	vector<cv::Point3d> pc1, pc2;
	vector<cv::Point2f> correspImg1Pt, correspImg2Pt;	

	// Get 3D locations of points relative to first camera
	TriangulatePoints(pts1, pts2, K, Kinv, P0, P1, pc1, correspImg1Pt);
	
	// Transfer 3D locations so that they are relative to second camera
	cv::Mat C1inv;
	C1inv = C1.inv();
	
	// src, dst, transform : if C1 represents transform from world to cam 1...
	transfer3DPoints(pc1, pc2, C1);	// trying C1inv...
	
	//Mat R1, t1;
	//decomposeTransform(C, R1, t1);
	//double deg1 = getRotationInDegrees(R1);
	//printf("%s << Forwards rotation = %f\n", __FUNCTION__, deg1);

	
	//printf("%s << DEBUG %d\n", __FUNCTION__, 2);
	//Cinv = C.inv();
	//transformationToProjection(Cinv, P1b);
	
	//Mat R2, t2;
	//decomposeTransform(Cinv, R2, t2);
	//double deg2 = getRotationInDegrees(R2);
	//printf("%s << Backwards rotation = %f\n", __FUNCTION__, deg2);
	
	//printf("%s << DEBUG %d\n", __FUNCTION__, 3);
	//TriangulatePoints(pts2, pts1, K, Kinv, P0, P1b, pc2, correspImg2Pt);

	//printf("%s << DEBUG %d\n", __FUNCTION__, 4);
	
	int inFront1 = 0, inFront2 = 0;
	
	for (unsigned int iii = 0; iii < pc1.size(); iii++) {
		
		// Check z-coordinate in both clouds to make sure it's > 0
		
		//printf("%s << pc1 = (%f, %f, %f); pc2 = (%f, %f, %f)\n", __FUNCTION__, pc1.at(iii).x, pc1.at(iii).y, pc1.at(iii).z, pc2.at(iii).x, pc2.at(iii).y, pc2.at(iii).z);
		
		if (pc1.at(iii).z > 0) {
			inFront1++;
		}
		
		if (pc2.at(iii).z > 0) {
			inFront2++;
		}
		
		if ((pc1.at(iii).z > 0) && (pc2.at(iii).z > 0)) {
			retVal++;
		}
		
	}
	
	//printf("%s << Pts in front of cams: %d, %d\n", __FUNCTION__, inFront1, inFront2);
	
	return retVal;
	
}

void convertPoint3dToMat(const cv::Point3d& src, cv::Mat& dst) {
	dst = cv::Mat::zeros(4, 1, CV_64FC1);
	
	dst.at<double>(3, 0) = 1.0;

	dst.at<double>(0,0) = src.x;
	dst.at<double>(1,0) = src.y;
	dst.at<double>(2,0) = src.z;
}



double calcGeometryDistance(cv::Point2f& pt1, cv::Point2f& pt2, cv::Mat& mat, int distMethod) {
	
	cv::Mat p1, p2;
	p1 = cv::Mat::ones(3,1,CV_64FC1);
	p2 = cv::Mat::ones(3,1,CV_64FC1);
	
	p1.at<double>(0,0) = (double) pt1.x;
	p1.at<double>(1,0) = (double) pt1.y;
	
	p2.at<double>(0,0) = (double) pt2.x;
	p2.at<double>(1,0) = (double) pt2.y;
	
	cv::Mat p2t;
	transpose(p2, p2t);
	
	double dist = 0.0;
	
	double ri;
	
	cv::Mat A;
	
	A = p2t * mat * p1; 
	ri = A.at<double>(0,0); 
	
	double r = abs(ri);	// not sure if this is correct and/or needed
			
	double rx, ry, rxd, ryd;
	
	rx = mat.at<double>(0,0) * pt2.x + mat.at<double>(1,0) * pt2.y + mat.at<double>(2,0);
	ry = mat.at<double>(0,1) * pt2.x + mat.at<double>(1,1) * pt2.y + mat.at<double>(2,1);
	rxd = mat.at<double>(0,0) * pt1.x + mat.at<double>(0,1) * pt1.y + mat.at<double>(0,2);
	ryd = mat.at<double>(1,0) * pt1.x + mat.at<double>(1,1) * pt1.y + mat.at<double>(1,2);
	
	double e1, e2;
	
	e1 = r / pow( pow(rx, 2.0) + pow(ry, 2.0) , 0.5);
	e2 = r / pow( pow(rxd, 2.0) + pow(ryd, 2.0) , 0.5);
	
	double de;
	
	de = pow(pow(e1, 2.0) + pow(e2, 2.0), 0.5);
	
	double ds;
	
	ds = r * pow((1 / (pow(rx, 2.0) + pow(ry, 2.0) + pow(rxd, 2.0) + pow(ryd, 2.0))), 0.5);
	
	switch (distMethod) {
		case SAMPSON_DISTANCE:
			dist = ds;
			break;
		case ALGEBRAIC_DISTANCE: 
			dist = r;
			break;
		case EPIPOLAR_DISTANCE:
			dist = de;
			break;
		case LOURAKIS_DISTANCE:
			dist = lourakisSampsonError(pt1, pt2, mat);
			break;
		default:
			break;
	}
	
	return dist;
}

double lourakisSampsonError(cv::Point2f& pt1, cv::Point2f& pt2, cv::Mat& H) {
	
	double error = 0.0;
	
	double t1, t10, t100, t104, t108, t112, t118, t12, t122, t125, t126, t129, t13, t139, t14, t141, t144, t15, t150, t153, t161, t167, t17, t174, t18, t19, t193, t199;
	double t2, t20, t201, t202, t21, t213, t219, t22, t220, t222, t225, t23, t236, t24, t243, t250, t253, t26, t260, t27, t271, t273, t28, t29, t296;
	double t3, t30, t303, t31, t317, t33, t331, t335, t339, t34, t342, t345, t35, t350, t354, t36, t361, t365, t37, t374, t39;
	double t4, t40, t41, t42, t43, t44, t45, t46, t47, t49, t51, t57;
	double t6, t65, t66, t68, t69;
	double t7, t72, t78;
	double t8, t86, t87, t90, t95;
	
	double h[9];
	
	for (int iii = 0; iii < 3; iii++) {
		for (int jjj = 0; jjj < 3; jjj++) {
			h[3*iii + jjj] = H.at<double>(iii,jjj);
		}
	}
	
	double m1[2], m2[2];
	
	m1[0] = (double) pt1.x;
	m1[1] = (double) pt1.y;
	m2[0] = (double) pt2.x;
	m2[1] = (double) pt2.y;
	
	t1 = m2[0];
	t2 = h[6];
	t3 = t2*t1;
	t4 = m1[0];
	t6 = h[7];
	t7 = t1*t6;
	t8 = m1[1];
	t10 = h[8];
	t12 = h[0];
	t13 = t12*t4;
	t14 = h[1];
	t15 = t14*t8;
	t17 = t3*t4+t7*t8+t1*t10-t13-t15-h[2];
	t18 = m2[1];
	t19 = t18*t18;
	t20 = t2*t2;
	t21 = t19*t20;
	t22 = t18*t2;
	t23 = h[3];
	t24 = t23*t22;
	t26 = t23*t23;
	t27 = t6*t6;
	t28 = t19*t27;
	t29 = t18*t6;
	t30 = h[4];
	t31 = t29*t30;
	t33 = t30*t30;
	t34 = t4*t4;
	t35 = t20*t34;
	t36 = t2*t4;
	t37 = t6*t8;
	t39 = 2.0*t36*t37;
	t40 = t36*t10;
	t41 = 2.0*t40;
	t42 = t8*t8;
	t43 = t42*t27;
	t44 = t37*t10;
	t45 = 2.0*t44;
	t46 = t10*t10;
	t47 = t21-2.0*t24+t26+t28-2.0*t31+t33+t35+t39+t41+t43+t45+t46;
	t49 = t12*t12;
	t51 = t6*t30;
	t57 = t20*t2;
	t65 = t1*t1;
	t66 = t65*t20;
	t68 = t65*t57;
	t69 = t4*t10;
	t72 = t2*t49;
	t78 = t27*t6;
	t86 = t65*t78;
	t87 = t8*t10;
	t90 = t65*t27;
	t95 = -2.0*t49*t18*t51-2.0*t3*t12*t46-2.0*t1*t57*t12*t34-2.0*t3*t12*t33+t66
	*t43+2.0*t68*t69+2.0*t72*t69-2.0*t7*t14*t46-2.0*t1*t78*t14*t42-2.0*t7*t14*t26+
	2.0*t86*t87+t90*t35+2.0*t49*t6*t87;
	t100 = t14*t14;
	t104 = t100*t2;
	t108 = t2*t23;
	t112 = t78*t42*t8;
	t118 = t57*t34*t4;
	t122 = t10*t26;
	t125 = t57*t4;
	t126 = t10*t19;
	t129 = t78*t8;
	t139 = -2.0*t57*t34*t18*t23+2.0*t100*t6*t87+2.0*t104*t69-2.0*t100*t18*t108+
	4.0*t36*t112+6.0*t43*t35+4.0*t118*t37+t35*t28+2.0*t36*t122+2.0*t125*t126+2.0*
	t129*t126+2.0*t37*t122-2.0*t78*t42*t18*t30+t43*t21;
	t141 = t10*t33;
	t144 = t46*t18;
	t150 = t46*t19;
	t153 = t46*t10;
	t161 = t27*t27;
	t167 = 2.0*t36*t141-2.0*t144*t108+2.0*t37*t141+t66*t33+t150*t27+t150*t20+
	4.0*t37*t153+6.0*t43*t46+4.0*t112*t10+t43*t33+t161*t42*t19+t43*t26+4.0*t36*t153
	;
	t174 = t20*t20;
	t193 = 6.0*t35*t46+4.0*t10*t118+t35*t33+t35*t26+t174*t34*t19+t100*t27*t42+
	t100*t20*t34+t100*t19*t20+t90*t46+t65*t161*t42+t90*t26+t49*t27*t42+t49*t20*t34+
	t49*t19*t27;
	t199 = t34*t34;
	t201 = t12*t23;
	t202 = t14*t30;
	t213 = t42*t42;
	t219 = t66*t46+t100*t26+t46*t100+t174*t199-2.0*t201*t202-2.0*t144*t51+t46*
	t26+t65*t174*t34+t49*t33+t49*t46+t46*t33+t161*t213-2.0*t7*t14*t20*t34;
	t220 = t1*t27;
	t222 = t36*t8;
	t225 = t7*t14;
	t236 = t4*t6*t8;
	t243 = t3*t12;
	t250 = t46*t46;
	t253 = t1*t20;
	t260 = -4.0*t220*t14*t222-4.0*t225*t40-4.0*t220*t15*t10+2.0*t90*t40+2.0*
	t225*t24+2.0*t72*t236-2.0*t3*t12*t27*t42-4.0*t243*t44+2.0*t66*t44+2.0*t243*t31+
	t250+2.0*t68*t236-4.0*t253*t12*t236-4.0*t253*t13*t10;
	t271 = t4*t20;
	t273 = t8*t18;
	t296 = t10*t18;
	t303 = 2.0*t104*t236-2.0*t35*t31+12.0*t35*t44+2.0*t125*t37*t19-4.0*t271*t6*
	t273*t23+2.0*t36*t37*t26+2.0*t36*t129*t19-4.0*t36*t27*t273*t30+2.0*t36*t37*t33+
	12.0*t36*t43*t10+12.0*t36*t37*t46-4.0*t271*t296*t23+2.0*t36*t126*t27;
	t317 = t18*t14;
	t331 = t14*t2;
	t335 = t12*t18;
	t339 = t220*t18;
	t342 = t7*t30;
	t345 = t317*t6;
	t350 = -4.0*t31*t40-2.0*t43*t24+2.0*t37*t126*t20-4.0*t44*t24-4.0*t27*t8*
	t296*t30-2.0*t253*t317*t30-2.0*t65*t2*t23*t6*t30+2.0*t3*t23*t14*t30-2.0*t12*t19
	*t331*t6+2.0*t335*t331*t30-2.0*t201*t339+2.0*t201*t342+2.0*t201*t345+2.0*t86*
	t222;
	t354 = 1/(t95+t139+t167+t193+t219+t260+t303+t350);
	t361 = t22*t4+t29*t8+t296-t23*t4-t30*t8-h[5];
	t365 = t253*t18-t3*t23-t335*t2+t201+t339-t342-t345+t202;
	t374 = t66-2.0*t243+t49+t90-2.0*t225+t100+t35+t39+t41+t43+t45+t46;

	error = sqrt((t17*t47*t354-t361*t365*t354)*t17+(-t17*t365*t354+t361*t374*
	t354)*t361);

	return error;
}	

double calcInlierGeometryDistance(vector<cv::Point2f>& points1, vector<cv::Point2f>& points2, cv::Mat& mat, cv::Mat& mask, int distMethod) {
	
	double error = 0.0;
	
	int inlierCounter = 0;

	for (unsigned int iii = 0; iii < points1.size(); iii++) {
		if (mask.at<char>(iii, 0) > 0) {
			
			error += calcGeometryDistance(points1.at(iii), points2.at(iii), mat, distMethod);
			
			inlierCounter++;
		}
	}
	
	error /= (double) inlierCounter;

	return error;
}

double calcGeometryScore(vector<cv::Point2f>& points1, vector<cv::Point2f>& points2, cv::Mat& F, cv::Mat& Fmask, cv::Mat& H, cv::Mat& Hmask) {
	double gScore = 0.0;
	
	int inliers_H = 0, inliers_F = 0;
	
	/*
	for (int iii = 0; iii < points1.size(); iii++) {
		if (Hmask.at<char>(iii, 0) > 0) {
			inliers_H++;
		}
		
		if (Fmask.at<char>(iii, 0) > 0) {
			inliers_F++;
		}		
	}
	* */
	
	inliers_H = countNonZero(Hmask);
	inliers_F = countNonZero(Fmask);
	
	printf("%s << Inliers: %d (H), %d (F)\n", __FUNCTION__, inliers_H, inliers_F);
	
	double sampson_H, sampson_F;
	
	sampson_H = calcSampsonError(points1, points2, H, Hmask);
	sampson_F = calcSampsonError(points1, points2, F, Fmask);
	
	double sampson_H2 = 0.0, sampson_F2 = 0.0;
	
	printf("%s << Sampson errors: %f (H), %f (F)\n", __FUNCTION__, sampson_H, sampson_F);
	
	for (unsigned int iii = 0; iii < points1.size(); iii++) {
		sampson_H2 += calcSampsonDistance(points1.at(iii), points2.at(iii), H) / (double)points1.size();
		sampson_F2 += calcSampsonDistance(points1.at(iii), points2.at(iii), F) / (double)points1.size();
	}
	
	printf("%s << Sampson errors2: %f (H), %f (F)\n", __FUNCTION__, sampson_H2, sampson_F2);
	
	
	gScore = (((double) inliers_F) / ((double) inliers_H)) * ( sampson_H / pow(sampson_F, 0.5));
	
	printf("%s << gScore = %f\n", __FUNCTION__, gScore);
	
	return gScore;
}

double calcFrameScore(double geomScore, int numFeatures, int numTracks) {
	
	double frameScore;
	
	frameScore = geomScore * ((double) numTracks) / ((double) numFeatures);
	
	return frameScore;
}

double normalizedGRICdifference(std::vector<cv::Point2f>& pts1, std::vector<cv::Point2f>& pts2, cv::Mat& F, cv::Mat& H, cv::Mat& mask_F, cv::Mat& mask_H, double& F_GRIC, double& H_GRIC) {
	
	double lambda_3 = 2.00;
	double r = 4.00;	// assuming always with 2-frames
	
	H_GRIC = calculateGRIC(pts1, pts2, H, mask_H, 2, 8, r, lambda_3, LOURAKIS_DISTANCE);
	F_GRIC = calculateGRIC(pts1, pts2, F, mask_F, 3, 7, r, lambda_3, SAMPSON_DISTANCE);

	return abs(F_GRIC - H_GRIC) / H_GRIC;
}

double calculateGRIC(std::vector<cv::Point2f>& pts1, std::vector<cv::Point2f>& pts2, cv::Mat& rel, cv::Mat& mask, int d, double k, double r, double lambda_3, int distMethod) {
	
	double retVal = 0.00;

	int n = int(pts1.size()); // countNonZero(mask);
	
	double lambda_1 = log(r);
	double lambda_2 = log(r*((double)n));
	
	double *e_vals;
	e_vals = new double[n];
	
	// gotta calculate sig and e
	
	// they calculate 'e' using:
	//		for F: point to epipolar line cost
	//		for H: symmetric transfer error
	
	double e_mean = 0.00, sig = 0.00;
	
	for (unsigned int iii = 0; iii < pts1.size(); iii++) {
		
		e_vals[iii] = calcGeometryDistance(pts1.at(iii), pts2.at(iii), rel, distMethod);
		e_mean += e_vals[iii] / ((double) n);
		
	}
	
	// Calculate variance
	
	for (unsigned int iii = 0; iii < pts1.size(); iii++) {
		sig += pow((e_vals[iii] - e_mean), 2.0) / ((double) n);
	}
	
	for (unsigned int iii = 0; iii < pts1.size(); iii++) {
		retVal += calculateRho(e_vals[iii], sig, r, lambda_3, d);
	}
	
	retVal +=  lambda_1*((double) d)*n + lambda_2*((double) k);

	return retVal;
	
}

double calculateRho(double e, double sig, double r, double lambda_3, int d) {
	
	double val1 = pow(e, 2.0) / pow(sig, 2.0);
	double val2 = lambda_3 * (r - ((double) d));
	
	return std::min(val1, val2);
	
}
 
double calcSampsonError(vector<cv::Point2f>& points1, vector<cv::Point2f>& points2, cv::Mat& H, cv::Mat& Hmask) {
	double sampsonError = 0.0;
	
	int inlierCounter = 0;

	
	for (unsigned int iii = 0; iii < points1.size(); iii++) {
		if (Hmask.at<char>(iii, 0) > 0) {
			
			sampsonError += lourakisSampsonError(points1.at(iii), points2.at(iii), H);
			
			inlierCounter++;
		}
	}
	
	sampsonError /= (double) inlierCounter;

	return sampsonError;

}

// http://www.mathworks.com.au/help/toolbox/vision/ref/estimatefundamentalmatrix.html
double calcSampsonDistance(cv::Point2f& pt1, cv::Point2f& pt2, cv::Mat& F) {
	
	double dist;
	
	cv::Mat point1(1, 3, CV_64FC1), point2(1, 3, CV_64FC1);
	
	point1.at<double>(0, 0) = (double) pt1.x;
	point1.at<double>(0, 1) = (double) pt1.y;
	point1.at<double>(0, 2) = 1.0;
	
	cv::Mat t1;
	transpose(point1, t1);
	
	point2.at<double>(0, 0) = (double) pt2.x;
	point2.at<double>(0, 1) = (double) pt2.y;
	point2.at<double>(0, 2) = 1.0;
	
	cv::Mat a, b, c;
	
	a = point2 * F * t1;
	b = F * t1;
	c = point2 * F;
	
	double val1 = pow(a.at<double>(0, 0), 2.0);
	double val2 = 1.0 / ((pow(b.at<double>(0, 0), 2.0)) + (pow(b.at<double>(1, 0), 2.0)));
	double val3 = 1.0 / ((pow(c.at<double>(0, 0), 2.0)) + (pow(c.at<double>(1, 0), 2.0)));
	
	dist = val1 * (val2 + val3);
	
	return dist;
	
}

double calcGeometryScore(int numInliers_H, int numInliers_F, double sampsonError_H, double sampsonError_F) {
	double geometryScore = 0.0;
	
	geometryScore = ((double) numInliers_F / (double) numInliers_H) * sampsonError_H / pow(sampsonError_F, 0.5);
	
	return geometryScore;
}

void assignIntrinsicsToP0(cv::Mat& P0, const cv::Mat& K) {
	P0 = cv::Mat::zeros(3, 4, CV_64FC1);
	
	for (int iii = 0; iii < 3; iii++) {
		for (int jjj = 0; jjj < 3; jjj++) {
			P0.at<double>(iii,jjj) = K.at<double>(iii,jjj);
		}
	}
	
}

void estimateNewPose(vector<featureTrack>& tracks, cv::Mat& K, int idx, cv::Mat& pose) {
	// Find any tracks that have at least 2 "sightings" (so that 3d location is known)
	// Use 3D points and 2d locations to estimate R and t and then get pose
	
	vector<cv::Point3d> worldPoints;
	vector<cv::Point2f> imagePoints1, imagePoints2;
	
	cv::Mat blankCoeffs = cv::Mat::zeros(1, 8, CV_64FC1);
	
	for (unsigned int iii = 0; iii < tracks.size(); iii++) {
		if (tracks.at(iii).locations.size() >= 2) {
			
			int finalIndex = tracks.at(iii).locations.size()-1;
			
			if (((int)tracks.at(iii).locations.at(finalIndex).imageIndex) == idx) {
				//Point3d tmpPt(tracks.at(iii).xyzEstimate.x, tracks.at(iii).xyzEstimate.y, tracks.at(iii).xyzEstimate.z);
				worldPoints.push_back(tracks.at(iii).get3dLoc());
				cv::Point2f tmpPt2(tracks.at(iii).locations.at(finalIndex-1).featureCoord.x, tracks.at(iii).locations.at(finalIndex-1).featureCoord.y);
				imagePoints1.push_back(tmpPt2);
				tmpPt2 = cv::Point2f(tracks.at(iii).locations.at(finalIndex).featureCoord.x, tracks.at(iii).locations.at(finalIndex).featureCoord.y);
				imagePoints2.push_back(tmpPt2);
			}
			
		}
	}
	
	
	cv::Mat Rvec, R, t;

	vector<cv::Point3f> objectPoints;
	cv::Point3f tmpPt;
	
	for (unsigned int jjj = 0; jjj < worldPoints.size(); jjj++) {
		tmpPt = cv::Point3f((float) worldPoints.at(jjj).x, (float) worldPoints.at(jjj).y, (float) worldPoints.at(jjj).z);
		objectPoints.push_back(tmpPt);
	}
	
	//solvePnPRansac(InputArray objectPoints, InputArray imagePoints, InputArray cameraMatrix, InputArray distCoeffs, OutputArray rvec, OutputArray tvec, bool useExtrinsicGuess=false, int iterationsCount=100, float reprojectionError=8.0, int minInliersCount=100, OutputArray inliers=noArray() );
	solvePnPRansac(objectPoints, imagePoints2, K, blankCoeffs, Rvec, t);
	Rodrigues(Rvec, R);

	//compileTransform(pose, R, t);
	composeTransform(R, t, pose);
}

Quaterniond defaultQuaternion() {
	Quaterniond defQuaternion(1, 0, 0, 0);
	
	return defQuaternion;
}

void convertProjectionMatCVToEigen(const cv::Mat& mat, Eigen::Matrix< double, 3, 4 >& m) {
	
	m(0,0) = mat.at<double>(0,0);
	m(0,1) = mat.at<double>(0,1);
	m(0,2) = mat.at<double>(0,2);
	m(0,3) = mat.at<double>(0,3);
	
	m(1,0) = mat.at<double>(1,0);
	m(1,1) = mat.at<double>(1,1);
	m(1,2) = mat.at<double>(1,2);
	m(1,3) = mat.at<double>(1,3);
	
	m(2,0) = mat.at<double>(2,0);
	m(2,1) = mat.at<double>(2,1);
	m(2,2) = mat.at<double>(2,2);
	m(2,3) = mat.at<double>(2,3);
	
}

void convertProjectionMatEigenToCV(const Eigen::Matrix< double, 3, 4 >& m, cv::Mat& mat) {
	
	mat = cv::Mat::zeros(3, 4, CV_64FC1);
	
	mat.at<double>(0,0) = m(0,0);
	mat.at<double>(0,1) = m(0,1);
	mat.at<double>(0,2) = m(0,2);
	mat.at<double>(0,3) = m(0,3);
	
	mat.at<double>(1,0) = m(1,0);
	mat.at<double>(1,1) = m(1,1);
	mat.at<double>(1,2) = m(1,2);
	mat.at<double>(1,3) = m(1,3);
	
	mat.at<double>(2,0) = m(2,0);
	mat.at<double>(2,1) = m(2,1);
	mat.at<double>(2,2) = m(2,2);
	mat.at<double>(2,3) = m(2,3);
	
}

void ExtractPointCloud(vector<cv::Point3d>& cloud, vector<featureTrack>& trackVector) {
	for (unsigned int iii = 0; iii < trackVector.size(); iii++) cloud.push_back(trackVector.at(iii).get3dLoc());
}

bool findBestReconstruction(const cv::Mat& P0, cv::Mat& P1, cv::Mat& R, cv::Mat& t, const cv::SVD& svd, const cv::Mat& K, const vector<cv::Point2f>& pts1, const vector<cv::Point2f>& pts2, bool useDefault) {
	
	bool validity = false;
	
	if (pts1.size() != pts2.size())printf("%s << ERROR! Point vector size mismatch.\n", __FUNCTION__);
	
	int bestIndex = 0, bestScore = -1;
	cv::Mat t33, t_tmp[4], R_tmp[4], W, Z, Winv, I, negI, P1_tmp[4];
	cv::Mat Kinv;
	//printf("%s << DEBUG [%d].\n", __FUNCTION__, -2);
	//printf("%s << DEBUG [%d].\n", __FUNCTION__, -1);
	Kinv = K.inv();
	
	//printf("%s << DEBUG [%d].\n", __FUNCTION__, 0);
	
	cv::Mat zeroTrans;
	zeroTrans = cv::Mat::zeros(3, 1, CV_64F);	// correct dimension?
	
	I = cv::Mat::eye(3, 3, CV_64FC1);
	negI = -I;
	getWandZ(W, Winv, Z);
	
	//printf("%s << DEBUG [%d].\n", __FUNCTION__, 1);
	
	cv::Mat u_transpose;
	transpose(svd.u, u_transpose);
	
	cv::Mat rotation1;
	Rodrigues(I, rotation1);
	
	//printf("%s << DEBUG [%d].\n", __FUNCTION__, 2);

	//cout << "rotation1 = " << rotation1 << endl;
	
	float sig = float(rotation1.at<double>(0,1));
	float phi = float(rotation1.at<double>(0,2));

	//printf("%s << sig = %f; phi = %f\n", __FUNCTION__, sig, phi);

	cv::Point3d unitVec1(1.0*sin(sig)*cos(phi), 1.0*sin(sig)*cos(phi), 1.0*cos(sig));
	
	//printf("%s << DEBUG [%d].\n", __FUNCTION__, 3);

	cv::Mat uv1(unitVec1);
	
	//printf("%s << DEBUG [%d].\n", __FUNCTION__, 4);
	
	// Go through all four cases...
	for (int zzz = 0; zzz < 4; zzz++) {
		
		//printf("%s << Loop [%d]\n", __FUNCTION__, zzz);
		
		switch (zzz) {
			case 0:
				//t33 = v * W * SIGMA * vt;
				t33 = svd.u * Z * u_transpose;
				R_tmp[zzz] = svd.u * Winv * svd.vt;
				break;
			case 1:
				//t33 = v * W_inv * SIGMA * vt;
				t33 = negI * svd.u * Z * u_transpose;
				R_tmp[zzz] = svd.u * Winv * svd.vt;
				break;
			case 2:
				//t33 = v * W * SIGMA * vt;
				t33 = svd.u * Z * u_transpose;
				R_tmp[zzz] = svd.u * W * svd.vt;
				break;
			case 3:
				//t33 = v * W_inv * SIGMA * vt;
				t33 = negI * svd.u * Z * u_transpose;
				R_tmp[zzz] = svd.u * W * svd.vt;
				break;
			default:
				break;
		}
		
		// Converting 3x3 t mat to vector
		t_tmp[zzz] = cv::Mat::zeros(3, 1, CV_64F);
		t_tmp[zzz].at<double>(0,0) = -t33.at<double>(1,2);
		t_tmp[zzz].at<double>(1,0) = t33.at<double>(0,2);
		t_tmp[zzz].at<double>(2,0) = -t33.at<double>(0,1);
		
		// Get new P1 matrix
		findP1Matrix(P1_tmp[zzz], R_tmp[zzz], t_tmp[zzz]);
		
		// Find camera vectors
		cv::Mat rotation2;
		Rodrigues(R_tmp[zzz], rotation2);

		//cout << "rotation2 = " << rotation2 << endl;

		sig = float(rotation2.at<double>(0,1));
		phi = float(rotation2.at<double>(0,2));

		//printf("%s << sig = %f; phi = %f\n", __FUNCTION__, sig, phi);

		cv::Point3d unitVec2(1.0*sin(sig)*cos(phi), 1.0*sin(sig)*cos(phi), 1.0*cos(sig));

		cv::Mat uv2(unitVec2);

		//printf("%s << unitVec2 = (%f, %f, %f)\n", __FUNCTION__, unitVec2.x, unitVec2.y, unitVec2.z);
		
		int badCount = 0;
		
		vector<cv::Point3d> pointcloud;
		vector<cv::Point2f> correspImg1Pt;
		
		//printf("%s << DEBUG [%d].\n", __FUNCTION__, 7);
		
		TriangulatePoints(pts1, pts2, K, Kinv, P0, P1_tmp[zzz], pointcloud, correspImg1Pt);
		
		//printf("%s << DEBUG [%d]. (pointcloud.size() = %d)\n", __FUNCTION__, 8, pointcloud.size());
		
		cv::Mat Rs_k[2], ts_k[2];

		// Then check how many points are NOT in front of BOTH of the cameras
		for (unsigned int kkk = 0; kkk < pointcloud.size(); kkk++) {
			
			
			
			//printf("%s << DEBUG [%d][%d].\n", __FUNCTION__, kkk, 0);

			cv::Point3d rayVec1, rayVec2;

			Rs_k[0] = I;
			Rs_k[1] = R_tmp[zzz];

			ts_k[0] = zeroTrans;
			ts_k[1] = t_tmp[zzz];
			
			//printf("%s << DEBUG [%d][%d].\n", __FUNCTION__, kkk, 1);
			
			//cout << endl << "t_tmp[zzz] = " << t_tmp[zzz] << endl;

			//printf("%s << objpt = (%f, %f, %f)\n", __FUNCTION__, objpt.x, objpt.y, objpt.z);

			// so this is getting the displacements between the points and both cameras...
			rayVec1.x = pointcloud.at(kkk).x - ts_k[0].at<double>(0,0);
			rayVec1.y = pointcloud.at(kkk).y - ts_k[0].at<double>(1,0);
			rayVec1.z = pointcloud.at(kkk).z - ts_k[0].at<double>(2,0);

			rayVec2.x = pointcloud.at(kkk).x - ts_k[1].at<double>(0,0);
			rayVec2.y = pointcloud.at(kkk).y - ts_k[1].at<double>(1,0);
			rayVec2.z = pointcloud.at(kkk).z - ts_k[1].at<double>(2,0);
			
			/*
			if (kkk < 0) {
				printf("%s << pt(%d) = (%f, %f, %f)\n", __FUNCTION__, kkk, pointcloud.at(kkk).x, pointcloud.at(kkk).y, pointcloud.at(kkk).z);
				printf("%s << rayVec1(%d) = (%f, %f, %f)\n", __FUNCTION__, kkk, rayVec1.x, rayVec1.y, rayVec1.z);
				printf("%s << rayVec2(%d) = (%f, %f, %f)\n", __FUNCTION__, kkk, rayVec2.x, rayVec2.y, rayVec2.z);
			}
			* */
			
			//printf("%s << DEBUG [%d][%d].\n", __FUNCTION__, kkk, 2);

			//printf("%s << rayVec1 = (%f, %f, %f)\n", __FUNCTION__, rayVec1.x, rayVec1.y, rayVec1.z);
			//printf("%s << rayVec2 = (%f, %f, %f)\n", __FUNCTION__, rayVec2.x, rayVec2.y, rayVec2.z);

			//cin.get();

			double dot1, dot2;
			
			cv::Mat rv1(rayVec1), rv2(rayVec2);
			
			/*
			cout << endl << "uv1 = " << uv1 << endl;
			cout << endl << "rv1 = " << rv1 << endl;
			
			cout << endl << "uv2 = " << uv2 << endl;
			cout << endl << "rv2 = " << rv2 << endl;
			*/

			dot1 = uv1.dot(rv1);
			dot2 = uv2.dot(rv2);

			//printf("%s << DEBUG [%d][%d].\n", __FUNCTION__, kkk, 3);

			//printf("%s << dot1 = %f; dot2 = %f\n", __FUNCTION__, dot1, dot2);

			double mag1 = pow(pow(rayVec1.x, 2) + pow(rayVec1.y, 2) + pow(rayVec1.z, 2), 0.5);
			double mag2 = pow(pow(rayVec2.x, 2) + pow(rayVec2.y, 2) + pow(rayVec2.z, 2), 0.5);

			//printf("%s << mag1 = %f; mag2 = %f\n", __FUNCTION__, mag1, mag2);

			double ang1 = fmod(acos(dot1 / mag1), 2*M_PI);
			double ang2 = fmod(acos(dot2 / mag2), 2*M_PI);

			//printf("%s << ang1 = %f; ang2 = %f\n", __FUNCTION__, ang1, ang2);
			// just try to check if angle is greater than 90deg from where each camera is facing to the pt

			// get vector from each camera to the point

			// use dot product to determine angle
			if ((abs(ang1) < M_PI/2 ) || (abs(ang2) < M_PI/2)) {
				//printf("%s << ang1 = %f; ang2 = %f\n", __FUNCTION__, ang1, ang2);
			//if ((dot1 < 0.0) || (dot2 < 0.0)) {
				validity = false;
				badCount++;
			}

			if (rayVec1.z < 0) {
				//printf("%s << rayVec1.z < 0; abs(ang1) = %f\n", __FUNCTION__, abs(ang1));

			} else {
				//printf("%s << rayVec1.z >= 0; abs(ang1) = %f\n", __FUNCTION__, abs(ang1));

			}

			//cin.get();

		}

		printf("%s << badCount[%d] = %d\n", __FUNCTION__, zzz, badCount);
		
		if (zzz == 0) {
			bestScore = badCount;
		} else {
			if (badCount < bestScore) {
				bestScore = badCount;
				bestIndex = zzz;
			}
		}

		
	}
	
	if (bestScore == 0) {
		validity = true;
	}
	
	// Should be 2 - but try different ones...
	if (useDefault) {
		bestIndex = 2;
		printf("%s << Defaulting to model #%d\n", __FUNCTION__, bestIndex);
	}
	
	findP1Matrix(P1, R_tmp[bestIndex], t_tmp[bestIndex]);

	R_tmp[bestIndex].copyTo(R);
	t_tmp[bestIndex].copyTo(t);
	
	return validity;
}

void findCentroid(vector<featureTrack>& tracks, cv::Point3d& centroid, cv::Point3d& stdDeviation) {
	
	unsigned int triangulatedPoints = 0;
	
	for (unsigned int iii = 0; iii < tracks.size(); iii++) {
		if (tracks.at(iii).isTriangulated) {
			triangulatedPoints++;
		}
	}
	
	printf("%s << triangulatedPoints = %d\n", __FUNCTION__, triangulatedPoints);
	
	centroid = cv::Point3d(0.0, 0.0, 0.0);
	stdDeviation = cv::Point3d(0.0, 0.0, 0.0);
	
	for (unsigned int iii = 0; iii < tracks.size(); iii++) {
		if (tracks.at(iii).isTriangulated) {
			centroid.x += (tracks.at(iii).get3dLoc().x / ((double) triangulatedPoints));
			centroid.y += (tracks.at(iii).get3dLoc().y / ((double) triangulatedPoints));
			centroid.z += (tracks.at(iii).get3dLoc().z / ((double) triangulatedPoints));
		}
	}
	
	for (unsigned int iii = 0; iii < tracks.size(); iii++) {
		if (tracks.at(iii).isTriangulated) {
			stdDeviation.x += (pow((tracks.at(iii).get3dLoc().x - centroid.x), 2.0) / ((double) triangulatedPoints));
			stdDeviation.y += (pow((tracks.at(iii).get3dLoc().y - centroid.y), 2.0) / ((double) triangulatedPoints));
			stdDeviation.z += (pow((tracks.at(iii).get3dLoc().z - centroid.z), 2.0) / ((double) triangulatedPoints));
		}
		
	}
	
	stdDeviation.x = pow(stdDeviation.x, 0.5);
	stdDeviation.y = pow(stdDeviation.y, 0.5);
	stdDeviation.z = pow(stdDeviation.z, 0.5);
	
}

#endif