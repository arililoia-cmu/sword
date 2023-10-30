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
    PlayMode::Player* player;
    //glm::vec3 targetPosition;
    int distanceToPlayer;
};
class CheckIfPlayerExist:public Node{
    private:
        BlackBoard* status;
    public:
        CheckIfPlayerExist(BlackBoard* status):status(status){

        }
        virtual bool run() override{
            return false;
/*
            if(status->player==nullptr){
                std::cout<<"Player doesn't exist"<<std::endl;
                return false;
            }else{
                std::cout<<"Player exist"<<std::endl;
                return true;
            }*/
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
            if(status->distanceToPlayer>0){
                std::cout<<"Approaches to the position"<<std::endl;
                status->distanceToPlayer--;
                if(status->distanceToPlayer>1){
                	std::cout << "The person is now " << status->distanceToPlayer << " meters from position." << std::endl;
                }else if (status->distanceToPlayer == 1){
				    std::cout << "The person is now only one meter away from the position." << std::endl;
                }else{
				    std::cout << "The person is at the position." << std::endl;
                }
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
            return true;
        }
};
class BehaviorTree{
    private:
        BlackBoard* status;
        Node* root;
        PlayMode::Player *enemy;
    public:
        BehaviorTree(){

        }
        void Start(){

        }
        void SetRoot(Node* rt){
            root=rt;
        }
        void tick(){
            if(root!=nullptr){
                root->run();
            }
        }
};
void BehaviorTreeTestCase();
