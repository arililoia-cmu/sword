#include "BehaviorTree.hpp"
#include<iostream>

void BehaviorTreeTestCase(){
    BehaviorTree* bt=new BehaviorTree();

	Sequence* root = new Sequence, * sequence1 = new Sequence;
   // root->SetBehaviorTree(bt);
	Selector* selector1 = new Selector;
    //selector1->SetBehaviorTree(bt);
	BlackBoard* blackBoard = new BlackBoard{};
	CheckIfPlayerExist* checkExist = new CheckIfPlayerExist(blackBoard,true);
    //checkExist->SetBehaviorTree(bt);
	WalkToPlayerTask* approach = new WalkToPlayerTask(blackBoard);
    //approach->SetBehaviorTree(bt);
	AttackTask* attack = new AttackTask(blackBoard);
    //attack->SetBehaviorTree(bt);


	root->addChild(selector1);

	selector1->addChild(checkExist);
	selector1->addChild(sequence1);

	sequence1->addChild(approach);
	sequence1->addChild(attack);

    bt->SetRoot(root);

    for(int i=0;i<5;++i){
        bt->tick();
    }

	std::cout << std::endl << "Operation complete.  Behaviour tree exited." << std::endl;
	//std::cin.get();
}
