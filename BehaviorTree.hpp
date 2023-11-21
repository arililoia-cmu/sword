#pragma once

#include "Pawn.hpp"
#include "PrintUtil.hpp"

#include <iostream>
#include <list>
#include <ctime>

#include <glm/glm.hpp>

//Insruction: https://cplusplus.com/forum/general/141582/
class Node
{
    public:
        virtual bool run()=0;

};
class CompositeNode :public Node{
    private:
        std::list<Node*> children;

    public:
        const std::list<Node*>& getChildren() const{
            return children;
        }
        void addChild(Node* child){
            children.emplace_back(child);
        }
};
class Selector: public CompositeNode{
    public:
        virtual bool run() override{
            for(Node* child:getChildren()){
                if(child->run()){
                    return true;
                }
            }
            return false;
        }
};
class Sequence: public CompositeNode{
    public:
        virtual bool run() override{
            for(Node* child : getChildren() ){
                if(!child->run())
                    return false;
            }
            return true;
        }
};
struct BlackBoard{
//    PlayMode::Pawn* player;
    //glm::vec3 targetPosition;
    float distanceToPlayer;
    PawnControl control;
    Pawn* player;
    Pawn* enemy;
};
class CheckIfPlayerExist:public Node{
    private:
        BlackBoard* status=nullptr;
        bool isNegative=false;
    public:
        CheckIfPlayerExist(BlackBoard* status,bool isNegative):status(status),isNegative(isNegative){
        }
        virtual bool run() override{
            if(status->player==nullptr){
                DEBUGOUT << "Player doesn't exist"<<std::endl;
                return isNegative;
            }else{
                glm::vec3 distance=status->player->transform->position - status->enemy->transform->position;
                float length=glm::length(distance);
                status->distanceToPlayer=length;
                return !isNegative;
            }
        }
        
};

class CheckDistance:public Node{
    private:
        BlackBoard* status=nullptr;
        bool isNegative=false;
        float distanceMax=100.0f;
        float distanceMin=0.0f;
    public:
        CheckDistance(BlackBoard* status,bool isNegative,float distance0,float distance1):status(status),isNegative(isNegative),distanceMax(distance0),distanceMin(distance1){
        }
        virtual bool run() override{
            if(status->distanceToPlayer<distanceMax && status->distanceToPlayer>distanceMin){
            //    std::cout<<status->distanceToPlayer<<std::endl;
                return !isNegative;
            }else{
            //    std::cout<<status->distanceToPlayer<<std::endl;
                return isNegative;
            }
        }
        
};
class ActionNode:public Node{
    private:
        int cd=5;
        time_t timestamp=0;
    public:
        bool CheckTime(){
            if(cd==0)return true;
            time_t t=time(0);
            time_t delta=t-timestamp;
            DEBUGOUT<<delta<<std::endl;
            if(delta>cd){
                return true;
            }else{
                return false;
            }
        }
        void SetCDTime(int i){
            cd=i;
        }
        void RegisterTime(){
            timestamp=time(0);
        }
};

class WalkToPlayerTask: public ActionNode{
    private:
        BlackBoard* status=nullptr;
    public:
        WalkToPlayerTask(BlackBoard* status):status(status){
            
        }
        virtual bool run() override{

            if(status->distanceToPlayer>4){
            //    std::cout<<"Walking"<<std::endl;
            	constexpr float EnemySpeed = 2.0f;
		        glm::vec3 diff = status->player->transform->position - status-> enemy->transform->position;
		        glm::vec3 emove = glm::normalize(diff) * EnemySpeed ;

                status->control.move=emove;
                float angle = std::atan2(diff.y, diff.x);
                status->control.rotate=	angle;
            }else{
            //    std::cout<<"Already There"<<std::endl;
            }
            return true;
        }
};
class AttackTask:public ActionNode{
    private:
        BlackBoard* status=nullptr;

    public:
        AttackTask(BlackBoard* status):status(status){

        }
        virtual bool run()override{
            if(CheckTime()){
            //    std::cout<<"AttackAction"<<status->control.attack<<std::endl;
            //    int temp=0;
            //    std::cin>>temp;
                status->control.attack=1;
                RegisterTime();
                return true;
            }else{
                return false;
            }

        }
};
class ParryTask:public ActionNode{
    private:
        BlackBoard* status=nullptr;
    public:
        ParryTask(BlackBoard* status):status(status){
            SetCDTime(0);
        }
        virtual bool run()override{
            if(CheckTime()){
                DEBUGOUT<<"ParryAction"<<std::endl;

                status->control.parry=1;
                RegisterTime();
                return true;
            }else{
                return false;
            }

        }
};
class AttackInterrupt:public Sequence{
    private:
        BlackBoard* status=nullptr;
    public:
        AttackInterrupt(BlackBoard* status):status(status){

        }

        bool IsActivated(){
            if(status->player->gameplay_tags=="attack"){
                DEBUGOUT<<"Interrupt Activated!"<<std::endl;
                this->run();
                return true;
            }else{
                return false;
            }
        }
};
class BehaviorTree{
    private:
        BlackBoard* status=nullptr;
        Node* root=nullptr;
        AttackInterrupt* attack_ipt=nullptr;
    public:
        BehaviorTree(){

        }
        void InitInterrupt(){
            attack_ipt=new AttackInterrupt(status);
            CheckDistance* checkDistance=new CheckDistance(status,false,4.5f,0.0f);
            ParryTask* parryTask=new ParryTask(status);
            attack_ipt->addChild(checkDistance);
            attack_ipt->addChild(parryTask);
        }
        void Init(){
            DEBUGOUT<<"AI Initialize Start"<<std::endl;
        	Sequence* rt = new Sequence;
            Sequence* sequence1 = new Sequence;
            Selector* selector2=new Selector;
            Sequence* atkSequence=new Sequence;
	        Selector* selector1 = new Selector;
            Sequence* approachSequence=new Sequence;
	        BlackBoard* blackBoard = new BlackBoard{};
            //blackBoard->distanceToPlayer=5;
	        CheckIfPlayerExist* checkExist = new CheckIfPlayerExist(blackBoard,true);
	        WalkToPlayerTask* approach = new WalkToPlayerTask(blackBoard);
	        AttackTask* attack = new AttackTask(blackBoard);
            CheckDistance* checkDistanceAttack=new CheckDistance(blackBoard,false,4.5f,0.0f);
            CheckDistance* checkDistanceApproach=new CheckDistance(blackBoard,false,20.0f,4.5f);
            status=blackBoard;

        	rt->addChild(selector1);
	        selector1->addChild(checkExist);
	        selector1->addChild(sequence1);
            sequence1->addChild(selector2);
	        selector2->addChild(approachSequence);
            selector2-> addChild(atkSequence);
            approachSequence->addChild(checkDistanceApproach);
            approachSequence->addChild(approach);
            atkSequence->addChild(checkDistanceAttack);
            atkSequence->addChild(attack);
            SetRoot(rt);
            InitInterrupt();
            DEBUGOUT<<"AI Initialize End"<<std::endl;
        }
        void Start(){

        }
        void SetRoot(Node* rt){
            root=rt;
        }
        void SetPlayer(Pawn* input){
            status->player=input;
        }
        void SetEnemy(Pawn* input){
            status->enemy=input;
        }

        void tick(){
            if(attack_ipt!=nullptr){
                if(attack_ipt->IsActivated()){
                    attack_ipt->run();
                    return;
                }
            }
            if(root!=nullptr){
            //    DEBUGOUT<<"AI Thinking Start----------"<<std::endl;
                root->run();
            //    DEBUGOUT<<"AI Thinking End---------"<<std::endl;
            }
        }
        PawnControl& GetControl()
		{
            return status->control;
        }
};
void BehaviorTreeTestCase();
