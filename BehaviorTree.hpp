#pragma once
#include<iostream>
#include<list>
#include"PlayMode.hpp"
#include <glm/glm.hpp>
//Insruction: https://cplusplus.com/forum/general/141582/
class Node{
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
    PlayMode::Control control;
    PlayMode::Pawn* player;
    PlayMode::Pawn* enemy;
};
class CheckIfPlayerExist:public Node{
    private:
        BlackBoard* status;
        bool isNegative;
    public:
        CheckIfPlayerExist(BlackBoard* status,bool isNegative):status(status),isNegative(isNegative){
        }
        virtual bool run() override{
            if(status->player==nullptr){
                std::cout<<"Player doesn't exist"<<std::endl;
                return isNegative;
            }else{
                glm::vec3 distance=status->player->transform->position - status->enemy->transform->position;
                float length=glm::length(distance);
                status->distanceToPlayer=length;
                return !isNegative;
            }
        }
        
};
class WalkToPlayerTask: public Node{
    private:
        BlackBoard* status;
    public:
        WalkToPlayerTask(BlackBoard* status):status(status){
            
        }
        virtual bool run() override{
            std::cout<<"Walking"<<std::endl;
            if(status->distanceToPlayer>1){
                std::cout<<"Approaches to the position"<<std::endl;

            	constexpr float EnemySpeed = 2.0f;
		        glm::vec3 diff = status->player->transform->position - status-> enemy->transform->position;
		        glm::vec3 emove = glm::normalize(diff) * EnemySpeed ;

                status->control.move=emove;
                float angle = std::atan2(diff.y, diff.x);
                status->control.rotate=	angle;
            }
            return true;
        }
};
class AttackTask:public Node{
    private:
        BlackBoard* status;

    public:
        AttackTask(BlackBoard* status):status(status){

        }
        virtual bool run()override{
            std::cout<<"AttackAction"<<std::endl;
            status->control.attack=1;
            return true;
        }
};
class BehaviorTree{
    private:
        BlackBoard* status;
        Node* root;
        PlayMode::Pawn *enemy;
    public:
        BehaviorTree(){

        }
        void Init(){
            std::cout<<"AI Initialize Start"<<std::endl;
        	Sequence* rt = new Sequence, * sequence1 = new Sequence;
	        Selector* selector1 = new Selector;
	        BlackBoard* blackBoard = new BlackBoard{};
            //blackBoard->distanceToPlayer=5;
	        CheckIfPlayerExist* checkExist = new CheckIfPlayerExist(blackBoard,true);
	        WalkToPlayerTask* approach = new WalkToPlayerTask(blackBoard);
	        AttackTask* attack = new AttackTask(blackBoard);
            status=blackBoard;

        	rt->addChild(selector1);
	        selector1->addChild(checkExist);
	        selector1->addChild(sequence1);
	        sequence1->addChild(approach);
            sequence1->addChild(attack);
            SetRoot(rt);
            std::cout<<"AI Initialize End"<<std::endl;
        }
        void Start(){

        }
        void SetRoot(Node* rt){
            root=rt;
        }
        void SetPlayer(PlayMode::Pawn* input){
            status->player=input;
        }
        void SetEnemy(PlayMode::Pawn* input){
            status->enemy=input;
        }

        void tick(){
            if(root!=nullptr){
                std::cout<<"AI Thinking Start----------"<<std::endl;
                root->run();
                std::cout<<"AI Thinking End---------"<<std::endl;
            }
        }
        PlayMode::Control& GetControl(){
            return status->control;
        }
};
void BehaviorTreeTestCase();
