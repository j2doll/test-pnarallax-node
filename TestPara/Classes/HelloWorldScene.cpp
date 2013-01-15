
#include "HelloWorldScene.h"

USING_NS_CC; 


// Asteroid consts
const float HelloWorld::_KFILTERINGFACTOR_ = 0.1 ;
const float HelloWorld::_KRESTACCELX_ =  -0.6;
const float HelloWorld::_KMAXDIFFX_ = 0.2  ;
// shipLaser const
const int   HelloWorld::_KNUMLASERS_ = 5 ;
// asteroid const
const int HelloWorld::_KNUMASTEROIDS_ = 15 ;

CCScene* HelloWorld::scene()
{
	CCScene *scene = CCScene::create(); // 'scene' is an autorelease object

	HelloWorld* helloWorld = HelloWorld::create(); // 'layer' is an autorelease object

	scene->addChild(helloWorld); // add layer as a child to scene

	return scene; // return the scene
}

// on "init" you need to initialize your instance
bool HelloWorld::init()
{

	//////////////////////////////
	// 1. super init first
	if ( ! CCLayer::init() )
	{
		return false;
	}

	{
		_batchNode = NULL;
		_ship = NULL;

		_backgroundNode = NULL;  
		_spacedust1 = NULL;
		_spacedust2 = NULL;
		_planetsunrise = NULL;
		_galaxy = NULL;
		_spacialanomaly = NULL;
		_spacialanomaly2 = NULL;  

		_shipPointsPerSecY = 0;  

		_asteroids = NULL; 

		_nextAsteroid = 0;
		_nextAsteroidSpawn = 0;     

		_shipLasers = NULL;  

		_nextShipLaser = 0;

		_lives = 0;

		_gameOverTime = 0;
		_gameOver = 0; 
	}

	//--------------------------------------
   // Setup BatchNode
   _batchNode = CCSpriteBatchNode::batchNodeWithFile("Sprites.pvr.ccz") ;
   this->addChild(_batchNode) ;
   CCSpriteFrameCache::sharedSpriteFrameCache()->addSpriteFramesWithFile("Sprites.plist") ;
   // Setup Ship
   _ship = CCSprite::spriteWithSpriteFrameName("SpaceFlier_sm_1.png") ;
   CCSize winSize = CCDirector::sharedDirector()->getWinSize() ;
    _ship->setPosition(ccp(winSize.width * 0.1 , winSize.height * 0.5) ) ;
   _batchNode->addChild(_ship,100) ; // 6
  
   // 1) Create the CCParallaxNode
   _backgroundNode = CCParallaxNodeExtras::node() ;
   this->addChild(_backgroundNode,-1) ;
   // 2) Create the sprites we'll add to the CCParallaxNode
   _spacedust1 = CCSprite::spriteWithFile("bg_front_spacedust.png");
   _spacedust2 = CCSprite::spriteWithFile("bg_front_spacedust.png");
   _planetsunrise = CCSprite::spriteWithFile("bg_planetsunrise.png");
   _galaxy = CCSprite::spriteWithFile("bg_galaxy.png");
   _spacialanomaly = CCSprite::spriteWithFile("bg_spacialanomaly.png");
   _spacialanomaly2 = CCSprite::spriteWithFile("bg_spacialanomaly2.png");
  
   // 3) Determine relative movement speeds for space dust and background
   CCPoint dustSpeed = ccp(0.1, 0.1);
   CCPoint bgSpeed = ccp(0.05, 0.05);
  
   // 4) Add children to CCParallaxNode
   _backgroundNode->addChild(_spacedust1, 0 , dustSpeed , ccp(0,winSize.height/2) );
   _backgroundNode->addChild( _spacedust2, 0 , dustSpeed , ccp( _spacedust1->getContentSize().width,winSize.height/2));
   _backgroundNode->addChild( _galaxy, -1 , bgSpeed , ccp(0,winSize.height * 0.7));
   _backgroundNode->addChild( _planetsunrise, -1 , bgSpeed , ccp(600,winSize.height * 0));
   _backgroundNode->addChild( _spacialanomaly, -1 , bgSpeed , ccp(900,winSize.height * 0.3));
   _backgroundNode->addChild( _spacialanomaly2, -1 , bgSpeed , ccp(1500,winSize.height * 0.9));

   this->scheduleUpdate() ;

   // adding stars  
   HelloWorld::addChild(CCParticleSystemQuad::particleWithFile("Stars1.plist")) ;
   HelloWorld::addChild(CCParticleSystemQuad::particleWithFile("Stars2.plist")) ;
   HelloWorld::addChild(CCParticleSystemQuad::particleWithFile("Stars3.plist")) ;
  
   // We'll use accelerometer
   this->setAccelerometerEnabled(true);
  
   // Asteroids
   _asteroids = new CCArray(); 
   for(int i = 0; i < _KNUMASTEROIDS_; ++i) {
     CCSprite *asteroid = CCSprite::spriteWithSpriteFrameName("asteroid.png");
	 asteroid->setVisible(false);
     _batchNode->addChild(asteroid);
     _asteroids->addObject(asteroid);
   }
  
    // shipLasers
   _shipLasers = new CCArray(); 
   for ( int i = 0; i < _KNUMLASERS_; ++i ) 
   {
     CCSprite *shipLaser = CCSprite::spriteWithSpriteFrameName("laserbeam_blue.png");
     
	 shipLaser->setVisible(false); 

     _batchNode->addChild(shipLaser);

     _shipLasers->addObject(shipLaser);

   }
 
   this->setTouchEnabled(true); 
   
   // GameOver Stuff
   _lives = 3 ;
   double curTime = getTimeTick() ;

   // win when 30 seconds elapse
   _gameOverTime = curTime + 30000 ;
  
   // Adding sounds
   
    return true;
}

float HelloWorld::randomValueBetween( float low , float high ) {
  return (((float) rand() / 0xFFFFFFFF) * (high - low)) + low;
}

void HelloWorld::setInvisible(CCNode * node) {
  node->setVisible(false);
}

// A portable way to get millisecs
float HelloWorld::getTimeTick() {
  timeval time;
  gettimeofday(&time, NULL);
  unsigned long millisecs = (time.tv_sec * 1000) + (time.tv_usec / 1000);
  return (float) millisecs;
}  

// Accelerometer callback stuff
void HelloWorld::didAccelerate(CCAcceleration* pAccelerationValue) {
#define KSHIPMAXPOINTSPERSEC (winSize.height*0.5)        

  double rollingX = 0;
  
  // Cocos2DX inverts X and Y accelerometer depending on device orientation
  // in landscape mode right x=-y and y=x !!! (Strange and confusing choice)
  pAccelerationValue->x = pAccelerationValue->y ;
  rollingX = (pAccelerationValue->x * _KFILTERINGFACTOR_) + (rollingX * (1.0 - _KFILTERINGFACTOR_));    
  float accelX = pAccelerationValue->x - rollingX ;
  CCSize winSize = CCDirector::sharedDirector()->getWinSize();
  float accelDiff = accelX - _KRESTACCELX_;
  float accelFraction = accelDiff / _KMAXDIFFX_;
  _shipPointsPerSecY = KSHIPMAXPOINTSPERSEC * accelFraction;   
}


void HelloWorld::restartTapped( CCObject* object ) 
{
  CCDirector::sharedDirector()->replaceScene
     ( CCTransitionZoomFlipX::transitionWithDuration(0.5, this->scene()));
  // reschedule
  this->scheduleUpdate() ; 
}

void HelloWorld::update(float dt) {
  
  CCPoint backgroundScrollVert = ccp(-1000,0) ;
  _backgroundNode->setPosition(ccpAdd(_backgroundNode->getPosition(),ccpMult(backgroundScrollVert,dt))) ; 
  
  CCArray *spaceDusts = CCArray::arrayWithCapacity(2) ;
  spaceDusts->addObject(_spacedust1) ;
  spaceDusts->addObject(_spacedust2) ;
  for ( int ii = 0  ; ii <spaceDusts->count() ; ii++ ) {
    CCSprite * spaceDust = (CCSprite *)(spaceDusts->objectAtIndex(ii)) ;
    float xPosition = _backgroundNode->convertToWorldSpace(spaceDust->getPosition()).x  ;
    float size = spaceDust->getContentSize().width ;
    if ( xPosition < -size ) {
      _backgroundNode->incrementOffset(ccp(spaceDust->getContentSize().width*2,0),spaceDust) ; 
    }                                  
  } 
  
  CCArray *backGrounds = CCArray::arrayWithCapacity(4) ;
  backGrounds->addObject(_galaxy) ;
  backGrounds->addObject(_planetsunrise) ;
  backGrounds->addObject(_spacialanomaly) ;
  backGrounds->addObject(_spacialanomaly2) ;
  for ( int ii = 0 ; ii <backGrounds->count() ; ii++ ) {
    CCSprite * background = (CCSprite *)(backGrounds->objectAtIndex(ii)) ;
    float xPosition = _backgroundNode->convertToWorldSpace(background->getPosition()).x ;
    float size = background->getContentSize().width ;
    if ( xPosition < -size ) {
      _backgroundNode->incrementOffset(ccp(2000,0),background) ; 
    }                                   
  }
  
  // Accelerometer stuff moving ship
  CCSize winSize = CCDirector::sharedDirector()->getWinSize();
  float maxY = winSize.height - _ship->getContentSize().height/2;
  float minY = _ship->getContentSize().height/2;

  float diff = (_shipPointsPerSecY * dt) ;
  float newY = _ship->getPosition().y + diff;
  newY = MIN(MAX(newY, minY), maxY);
  _ship->setPosition(ccp(_ship->getPosition().x, newY));    

  // dealing with asteroids
  // Add to bottom of update loop
  float curTimeMillis = getTimeTick();
   if (curTimeMillis > _nextAsteroidSpawn) {

      float randSecs = randomValueBetween(0.20,1.0) * 1000 ; // We're millisecs now
    _nextAsteroidSpawn = randSecs + curTimeMillis;

    float randY = randomValueBetween(0.0,winSize.height);
    float randDuration = randomValueBetween(2.0,10.0);

    // CCSprite *asteroid = _asteroids->getObjectAtIndex(_nextAsteroid);
	CCSprite *asteroid = (CCSprite *) _asteroids->objectAtIndex( _nextAsteroid ); 

    _nextAsteroid++;

    if (_nextAsteroid >= _asteroids->count())
      _nextAsteroid = 0;

    asteroid->stopAllActions();
    
	asteroid->setPosition( ccp(winSize.width+asteroid->getContentSize().width/2, randY));
	asteroid->setVisible(true);
     asteroid->runAction ( 
          CCSequence::actions (
            CCMoveBy::actionWithDuration(randDuration,ccp(-winSize.width-asteroid->getContentSize().width,0)) ,
            CCCallFuncN::actionWithTarget(this,callfuncN_selector(HelloWorld::setInvisible)) ,
            NULL  // DO NOT FORGET TO TERMINATE WITH NULL (unexpected in C++)
                              ) ) ;
  }  

	// collision detection 
	// CCArray<CCSprite *>::CCMutableArrayIterator itAster , itLaser ;
  
	CCObject* itAster = NULL; 
	CCARRAY_FOREACH( _asteroids, itAster )
	{
		CCSprite *asteroid = (CCSprite *) itAster; 
		if ( ! asteroid->isVisible() )
		  continue ;

		CCObject* itLaser = NULL;
		CCARRAY_FOREACH( _shipLasers, itLaser )
		{
		  CCSprite *shipLaser = (CCSprite *) itLaser;
		  if ( ! shipLaser->isVisible() )
			continue ;

			if ( CCRect::CCRectIntersectsRect(shipLaser->boundingBox(), asteroid->boundingBox()) ) 
			{
				shipLaser->setVisible(false) ;
				asteroid->setVisible(false) ; 
				continue ;
			}
		}  

		if ( CCRect::CCRectIntersectsRect(_ship->boundingBox(), asteroid->boundingBox()) ) 
		{
			asteroid->setVisible(false);
			_ship->runAction( CCBlink::actionWithDuration(1.0, 9)) ;
			_lives-- ;
		}
  }
   
  // Game Over stuff
  if ( _lives <= 0 ) 
  {
    _ship->stopAllActions() ;
    _ship->setVisible(false) ;
    this->endScene(KENDREASONLOSE) ;
  } else if ( curTimeMillis >= _gameOverTime ) 
  {
    this->endScene(KENDREASONWIN) ;
  }

}

void HelloWorld::ccTouchesBegan(cocos2d::CCSet* touches, cocos2d::CCEvent* event)
{
  CCSize winSize = CCDirector::sharedDirector()->getWinSize() ; 
  
  CCSprite *shipLaser = (CCSprite *) _shipLasers->objectAtIndex(_nextShipLaser++);

  if ( _nextShipLaser >= _shipLasers->count() )
    _nextShipLaser = 0;

  shipLaser->setPosition( ccpAdd( _ship->getPosition(), ccp(shipLaser->getContentSize().width/2, 0)));
  shipLaser->setVisible(true) ;
  shipLaser->stopAllActions() ;
  shipLaser->runAction( 
        CCSequence::actions (
          CCMoveBy::actionWithDuration(0.5,ccp(winSize.width, 0)),
          CCCallFuncN::actionWithTarget(this,callfuncN_selector(HelloWorld::setInvisible)) ,
          NULL  // DO NOT FORGET TO TERMINATE WITH NULL
          ) ) ;
}

void HelloWorld::endScene( EndReason endReason ) {
  if ( _gameOver )
    return ;
  _gameOver = true ;
  
  CCSize winSize = CCDirector::sharedDirector()->getWinSize() ; 
  char message[10] = "You Win"  ;
  if ( endReason == KENDREASONLOSE )
    strcpy(message,"You Loose") ;
  CCLabelTTF *label = CCLabelTTF::labelWithString(message, "Courier", 64.0);
  label->setScale(0.1) ;
  label->setPosition( ccp( winSize.width/2 , winSize.height*0.6)) ;
  this->addChild(label) ;  
  
  CCLabelTTF *restartLabel = CCLabelTTF::labelWithString("Restart", "Courier", 64.0);
  CCMenuItemLabel *restartItem = 
	  CCMenuItemLabel::itemWithLabel(restartLabel, this, menu_selector(HelloWorld::restartTapped) );  

  restartItem->setScale(0.1) ;
  restartItem->setPosition( ccp( winSize.width/2 , winSize.height*0.4)) ;
  
  CCMenu *menu = CCMenu::menuWithItems(restartItem, NULL);
  menu->setPosition(CCPointZero);
  this->addChild(menu) ;

  // clear label and menu
  restartItem->runAction( CCScaleTo::actionWithDuration(0.5, 1.0)) ;
  label ->runAction( CCScaleTo::actionWithDuration(0.5, 1.0)) ;
  // Terminate update
  this->unscheduleUpdate() ;
}

void HelloWorld::menuCloseCallback(CCObject* pSender)
{
	CCDirector::sharedDirector()->end();
   //if ( _batchNode != NULL ) {
   //  delete _batchNode ;
   //  _batchNode = NULL ;
   //}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	exit(0);
#endif
}
