const assetPathPrefix = "https://www.figma.com/api/mcp/asset/0d8b1951-3112-43b5-8405-1788cc44f99b";
const imgFrame = `${assetPathPrefix}/764c9.svg`;
const imgFrame1 = `${assetPathPrefix}/1f9fe.svg`;

type IconProps = {
  className?: string;
  direction?: "down";
  eq?: "default";
  name?: "chevron";
  state?: "default";
  twoTone?: "no";
  value?: "default";
};

function Icon({ className, direction = "down", eq = "default", name = "chevron", state = "default", twoTone = "no", value = "default" }: IconProps) {
  return (
    <div className={className || "content-stretch flex items-center justify-center relative"} data-node-id="71:868">
      <div className="flex items-center justify-center relative shrink-0" data-node-id="71:869">
        <div className="-scale-y-100 flex-none rotate-180">
          <div className="relative size-[24px]" data-name="Frame">
            <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame} />
          </div>
        </div>
      </div>
    </div>
  );
}

export default function Frame145() {
  return (
    <div className="content-stretch flex flex-col items-center py-[5px] relative size-full" data-node-id="3180:17672">
      <div className="content-stretch flex items-center justify-between relative shrink-0 w-full" data-node-id="3180:17673">
        <div className="content-stretch flex gap-[10px] items-center relative shrink-0" data-node-id="3180:17674">
          <p className="[word-break:break-word] font-['Inter:Regular'] font-normal leading-[normal] not-italic relative shrink-0 text-[#8f8f8f] text-[14px] whitespace-nowrap" data-node-id="3180:17675">
            Media role
          </p>
          <div className="content-stretch flex items-center justify-center relative shrink-0" data-node-id="3180:17676" data-name="Icon">
            <div className="relative shrink-0 size-[18px]" data-node-id="I3180:17676;2139:8986">
              <img alt="" className="absolute block inset-0 max-w-none size-full" src={imgFrame1} />
            </div>
          </div>
        </div>
        <div className="bg-[#090909] content-stretch flex items-start justify-center p-[2px] relative rounded-[6px] shrink-0 w-[320px]" data-node-id="3180:17677" data-name="Dropdown">
          <div className="bg-[#191919] content-stretch flex flex-[1_0_0] gap-[10px] h-[32px] items-center min-w-px px-[14px] py-[2px] relative rounded-[4px]" data-node-id="3180:17678" data-name="option">
            <p className="[word-break:break-word] flex-[1_0_0] font-['Inter:Regular'] font-normal leading-[normal] min-w-px not-italic relative text-[#8f8f8f] text-[14px]" data-node-id="3180:17680">
              Video
            </p>
            <Icon className="content-stretch flex items-center justify-center relative shrink-0" />
          </div>
        </div>
      </div>
    </div>
  );
}
