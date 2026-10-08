#pragma once
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/video/tracking.hpp>
#include <array>
#include <limits>

namespace tracking {
// Only confirmed 3D measurements become the reference for candidate selection.
struct TemporalEyeDetector {
    cv::Rect lastFace;
    std::array<cv::Point, 2> previous{};
    bool initialized = false;
    unsigned missedFrames = 0;
    int estimatedEye = -1;
    cv::Mat currentGray, previousGray;
    std::array<cv::Point,2> flowEyes{};
    bool flowValid = false;
    unsigned estimatedChain = 0;
    cv::Rect fullFace;
    std::array<cv::Point,2> fullEyes{};
    cv::Mat annotated;
    cv::Rect candidateFace;
    void Commit(const std::array<cv::Point, 2>& eyes) {
        if (estimatedEye < 0) { fullFace = candidateFace; fullEyes = eyes; estimatedChain = 0; }
        else ++estimatedChain;
        if (!currentGray.empty()) previousGray = currentGray.clone();
        previous = eyes; lastFace = candidateFace; initialized = true; missedFrames = 0;
    }
    bool UpdateFlow(const cv::Mat& gray) {
        flowValid = false;
        if (!initialized || previousGray.empty() || previousGray.size()!=gray.size() || estimatedChain>=15) return false;
        std::vector<cv::Point2f> before, after, backward;
        for(auto p:previous) before.emplace_back(float(p.x),float(p.y));
        std::vector<unsigned char> forwardStatus, backwardStatus;
        std::vector<float> forwardError, backwardError;
        cv::calcOpticalFlowPyrLK(previousGray,gray,before,after,forwardStatus,forwardError,cv::Size(25,25),3);
        cv::calcOpticalFlowPyrLK(gray,previousGray,after,backward,backwardStatus,backwardError,cv::Size(25,25),3);
        const cv::Rect bounds(0,0,gray.cols,gray.rows);
        for(int k=0;k<2;++k) {
            if(!forwardStatus[k] || !backwardStatus[k] || forwardError[k]>30 ||
               cv::norm(before[k]-backward[k])>1.5 || cv::norm(before[k]-after[k])>50) return false;
            flowEyes[k]=cv::Point(cvRound(after[k].x),cvRound(after[k].y));
            if(!bounds.contains(flowEyes[k])) return false;
        }
        const double oldSeparation=cv::norm(previous[1]-previous[0]);
        const double newSeparation=cv::norm(flowEyes[1]-flowEyes[0]);
        if(flowEyes[0].x>=flowEyes[1].x || newSeparation<.7*oldSeparation || newSeparation>1.3*oldSeparation) return false;
        flowValid=true; return true;
    }
    bool EstimateFromOneEye(const cv::Rect& face, const std::vector<cv::Point>& candidates,
                            const cv::Size& imageSize, std::array<cv::Point,2>& eyes) {
        estimatedEye = -1;
        if (!initialized || fullFace.empty() || face.empty()) return false;
        std::array<cv::Point,2> expected;
        for(int k=0;k<2;++k) {
            expected[k].x=face.x+int((fullEyes[k].x-fullFace.x)*double(face.width)/fullFace.width);
            expected[k].y=face.y+int((fullEyes[k].y-fullFace.y)*double(face.height)/fullFace.height);
        }
        if(flowValid) expected=flowEyes;
        double best=std::max(12.,.18*face.width);
        int visible=-1;
        cv::Point anchor;
        for(auto p:candidates) {
            double dl=cv::norm(p-expected[0]), dr=cv::norm(p-expected[1]);
            if (std::abs(dl-dr)<8.) continue; // Ambiguous candidate between eyes.
            int index=dl<dr ? 0:1;
            double distance=std::min(dl,dr);
            if(distance<best) {best=distance;visible=index;anchor=p;}
        }
        if(visible<0) return false;
        eyes[visible]=anchor;
        eyes[1-visible]=anchor+(expected[1-visible]-expected[visible]);
        const cv::Rect bounds(0,0,imageSize.width,imageSize.height);
        if(!bounds.contains(eyes[0]) || !bounds.contains(eyes[1]) ||
           !face.contains(eyes[0]) || !face.contains(eyes[1])) return false;
        int separation=eyes[1].x-eyes[0].x;
        if(separation<.2*face.width || separation>.75*face.width) return false;
        estimatedEye=1-visible;
        return true;
    }
    bool Detect(const cv::Mat& bgr, cv::CascadeClassifier& faceClassifier,
                cv::CascadeClassifier& eyeClassifier,
                std::array<cv::Point, 2>& eyes, const char*& reason) {
        estimatedEye = -1;
        bool freshFace = false;
        const bool recovering = initialized && ++missedFrames >= 15;
        annotated = bgr.clone();
        cv::Mat gray;
        cv::cvtColor(bgr, gray, cv::COLOR_BGR2GRAY);
        cv::equalizeHist(gray, gray);
        currentGray=gray;
        UpdateFlow(gray);
        std::vector<cv::Rect> faces;
        faceClassifier.detectMultiScale(gray, faces, 1.1, 5, 0, cv::Size(60,60));
        cv::Rect face;
        if (!initialized) {
            if (faces.size() != 1) { reason = "need one face to initialize"; return false; }
            face = faces.front(); freshFace = true;
        } else {
            double best = std::numeric_limits<double>::max();
            const cv::Point center = lastFace.tl() + cv::Point(lastFace.width/2,lastFace.height/2);
            for (auto box : faces) {
                const cv::Point c = box.tl() + cv::Point(box.width/2,box.height/2);
                const double dist = cv::norm(c-center);
                if (dist < .75*lastFace.width && box.width > .6*lastFace.width && box.width < 1.7*lastFace.width && dist < best) {
                    face = box; best = dist; freshFace = true;
                }
            }
            if (face.empty() && recovering && faces.size() == 1) { face = faces.front(); freshFace = true; }
            if (face.empty()) face = lastFace & cv::Rect(0,0,gray.cols,gray.rows);
        }
        if (face.empty()) { reason = "face region missing"; return false; }
        candidateFace = face;
        cv::rectangle(annotated,face,cv::Scalar(255,255,0),2);
        const cv::Rect upper(face.x,face.y,face.width,std::max(1,int(face.height*.62)));
        std::vector<cv::Rect> candidates;
        // Previous position gates permit a less restrictive cascade threshold.
        eyeClassifier.detectMultiScale(gray(upper),candidates,1.08,initialized ? 3 : 6,0,
            cv::Size(std::max(18,face.width/10),std::max(12,face.height/12)));
        for (auto box:candidates) cv::rectangle(annotated,cv::Rect(box.tl()+upper.tl(),box.size()),cv::Scalar(0,255,255),1);
        double best = -std::numeric_limits<double>::max();
        bool found = false;
        for (size_t i=0;i<candidates.size();++i) for(size_t j=i+1;j<candidates.size();++j) {
            cv::Point l=candidates[i].tl()+cv::Point(candidates[i].width/2,candidates[i].height/2)+upper.tl();
            cv::Point r=candidates[j].tl()+cv::Point(candidates[j].width/2,candidates[j].height/2)+upper.tl();
            if(l.x>r.x) std::swap(l,r);
            int separation=r.x-l.x, vertical=std::abs(r.y-l.y);
            if(separation<.15*face.width || separation>.75*face.width || vertical>.45*face.height) continue;
            double score=candidates[i].area()+candidates[j].area()+10.*separation-15.*vertical;
            if(initialized) {
                auto expected = previous;
                if (recovering && !lastFace.empty()) {
                    for (int k=0; k<2; ++k) {
                        expected[k].x = face.x + int((previous[k].x-lastFace.x)*double(face.width)/lastFace.width);
                        expected[k].y = face.y + int((previous[k].y-lastFace.y)*double(face.height)/lastFace.height);
                    }
                }
                if(flowValid) expected=flowEyes;
                const double dl=cv::norm(l-expected[0]), dr=cv::norm(r-expected[1]);
                if(dl>std::max(35.,.4*face.width) || dr>std::max(35.,.4*face.width)) continue;
                score -= 50.*(dl+dr);
            }
            if(score>best) {best=score;eyes={{l,r}};found=true;}
        }
        if (!found && freshFace && initialized) {
            std::vector<cv::Point> centers;
            for(auto box:candidates) centers.push_back(box.tl()+cv::Point(box.width/2,box.height/2)+upper.tl());
            if(EstimateFromOneEye(face,centers,bgr.size(),eyes)) {
                reason=flowValid ? "one eye + tracked tilt: ESTIMATED" : "one eye + face: other eye ESTIMATED";
                return true;
            }
        }
        reason=found ? "OK" : "no continuous eye pair; holding";
        return found;
    }
};
}
